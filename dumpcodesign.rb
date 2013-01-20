#!/usr/bin/env ruby
require 'stringio'

class StringStream < StringIO
  def read_uint8
    getbyte
  end

  def read_uint16
    read(2).unpack("n").first
  end

  def read_uint32
    read(4).unpack("N").first
  end

  def read_uint64
    (read_uint32 << 32) + read_uint32
  end

  def read_uint128
    (read_uint64 << 64) + read_uint64
  end
end


module AppleCodeSignature
  class Blob
    class Magic
      attr_accessor :value
      def initialize(num)
        @value = (Magic === num)? num.value : num
      end

      def inspect
        "0x%x" % value
      end
    end

    attr_reader :magic, :size, :input

    def initialize(magic, data)
      @magic = Magic.new(magic)
      @input = data.is_a?(StringStream)? data : StringStream.new(data)
      @base = @input.pos-4
      yield self if block_given?
    end

    def parse
      if @input
        @size = input.read_uint32
        yield 
      end
      @input=nil
      return self
    end

    private
    def rest
      @input.read(size_left)
    end

    def size_left
      @size-(@input.pos-@base)
    end
  end

  class OpaqueBlob < Blob
    attr_reader :data, :base
    def parse
      super() do
        @data = rest()
      end
    end
  end

  class SuperBlob < Blob
    class ContentInfo
      attr_reader :unk, :offset
      def initialize(unk, offset)
        @unk = unk
        @offset = offset
      end
    end
    attr_reader :contents, :content_infos
    def parse
      super() do
        ncontent = @input.read_uint32
        @content_infos = Array.new(ncontent) {
          ContentInfo.new(@input.read_uint32, @input.read_uint32)
        }
        @contents = []
        @content_infos.each do |ci|
          @input.pos = @base+ci.offset
          @contents << AppleCodeSignature.parse(@input)
        end
      end
    end
  end

  # SuperBlob containing all the signing components that are usually
  # embedded within a main executable.
  # This is what we embed in Mach-O images. It is also what we use for detached
  # signatures for non-Mach-O binaries.
  class EmbeddedSignature < SuperBlob
  end

  # SuperBlob that contains all the data for all architectures of a
  # signature, including any data that is usually written to separate files.
  # This is the format of detached signatures if the program is capable of
  # having multiple architectures.
  # A DetachedSignatureBlob collects multiple architectures' worth of
  # EmbeddedSignatureBlobs into one, well, Super-SuperBlob.
  # This is what is used for Mach-O detached signatures.
  class DetachedSignature < SuperBlob
  end

  # A CodeDirectory
  class CodeDirectory < Blob
    # Types of cryptographic digests (hashes) used to hold code signatures
    # together.
    # 
    # Each combination of type, length, and other parameters is a separate
    # hash type; we don't understand "families" here.
    # 
    # These type codes govern the digest links that connect a CodeDirectory
    # to its subordinate data structures (code pages, resources, etc.)
    # They do not directly control other uses of hashes (such as the
    # hash-of-CodeDirectory identifiers used in requirements).
    HASHTYPES={
      0 => :NoHash,     # null value
      1 => :HashSha1,   # SHA-1
      2 => :HashSha256, # SHA-256
      32 => :HashPrestandardSkein160x256, # Skein, 160bits, 256bit pool
      33 => :HashPrestandardSkein256x512, # Skein, 256bits, 512bit pool
    }

	  CURRENT_VERSION = 0x20100	    # "version 2.1"
	  COMPATABILITY_LIMIT = 0x2F000	# "version 3 with wiggle room"
    EARLIEST_VERSION = 0x20001	  # earliest supported version
	  SUPPORTS_SCATTER = 0x20100	  # first version to support scatter option

    attr_reader :data
  	attr_reader :version        # uint32 compatibility version
  	attr_reader :flags          # uint32 setup and mode flags
  	attr_reader :hashOffset     # uint32 offset of hash slot element at index zero
  	attr_reader :identOffset    # uint32 offset of identifier string
  	attr_reader :nSpecialSlots	# uint32 number of special hash slots
  	attr_reader :nCodeSlots     # uint32 number of ordinary (code) hash slots
  	attr_reader :codeLimit      # uint32 limit to main image signature range
    attr_reader :hashSize       # size of each hash digest (bytes)
    attr_reader :hashType       # type of hash (kSecCodeSignatureHash* constants)
    attr_reader :spare1         # unused (must be zero)
    attr_reader	:pageSize       # log2(page size in bytes); 0 => infinite
    attr_reader :spare2         # uint32 unused (must be zero)
    attr_reader :scatterOffset  # uint32 offset of optional scatter vector (zero if absent)

    def parse
      super() do
        @vers = @input.read_uint32
        @flags = @input.read_uint32
        @hashOffset = @input.read_uint32
        @identOffset = @input.read_uint32
        @nSpecialSlots = @input.read_uint32
        @nCodeSlots = @input.read_uint32
        @codeLimit = @input.read_uint32
        @hashSize = @input.read_uint8
        @hashType = @input.read_uint8
        @spare1 = @input.read_uint8
        @pageSize = @input.read_uint8
        @spare2 = @input.read_uint32
        @scatterOffset = @input.read_uint32
        @data = rest()
      end
    end

    def version
      [@vers].pack("N").unpack("CCCC").join('.')
    end

    def hash_type
      HASHTYPES[@hashType] || :unknown
    end
  end

  # A collection of individual code requirements, indexed by requirement
  # type. This is used for internal requirement sets.
  class RequirementSet < SuperBlob
  end

  # An individual code requirement
  # This is actlually a small compiled expression.
  # csreq(1) can be used to decompile them
  class Requirement < Blob
    attr_reader :data, :decompiled
    def parse
      super() do
        @data=rest()
        @decompiled = self.class.decompile([@magic.value, @size, @data].pack("NNA*"))
      end
    end

    def self.decompile(data)
      csreq=IO.popen("csreq -r- -t", "r+")
      csreq.write(data)
      decompiled = csreq.read()
      csreq.close()
      return decompiled
    end
  end

  # The linkers produces a superblob of dependency records from its dylib inputs
  class LibraryDependencyBlob < OpaqueBlob
  end

  # Program Entitlements Dictionary
  class Entitlement < OpaqueBlob
  end

  class BlobWrapper < OpaqueBlob
  end

  class UnknownBlob < OpaqueBlob
  end

  FADEMAGIC = {
    0xfade0c00 => :Requirement,
    0xfade0c01 => :RequirementSet,
    0xfade0c02 => :CodeDirectory,
    0xfade0c05 => :LibraryDependencyBlob,
    0xfade0cc0 => :EmbeddedSignature,
    0xfade0cc1 => :DetachedSignature,
    0xfade0b01 => :BlobWrapper,
    0xfade7171 => :Entitlement,
  }

  def self.parse(data)
    obj = data.is_a?(StringStream)? data : StringStream.new(data)
    magic = obj.read_uint32
    n=FADEMAGIC[magic]
    if n.nil? and ((magic >> 16) == 0xFADE)
      n = :UnknownBlob
    end
    unless n.nil?
      blob=const_get(n).new(magic,obj) {|o| o.parse }
      return blob
    else
      raise "Invalid magic value"
    end
  end
end

if __FILE__ == $0
  require 'pp'
  ARGV.each do |fname|
    puts "[+] parsing: #{fname}"
    pp AppleCodeSignature.parse(File.read(fname))
  end
end
