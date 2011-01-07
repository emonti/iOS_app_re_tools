#!/usr/bin/env ruby
# uncrypt.rb by eric monti
#
# A utility for patching encrypted mach-o binaries with decrypted data.
#
# usage: uncrypt.rb exe_file dec_file lc_offset

LC_ENCRYPTION_INFO_CMD = 0x00000021
LC_ENCRYPTION_INFO_CMDSIZE = 20

if( ARGV.include?("-h") or not 
    (exe = ARGV.shift and decrypt = ARGV.shift and lc_offset = ARGV.shift))
  STDERR.puts "usage: #{File.basename $0} exe_file dec_file lc_offset"
  exit 1
end

lc_offset = lc_offset.to_i
decrypt_dat = File.read(decrypt)

File.open(exe, "r+") do |f|
  f.pos = lc_offset
  lc = f.read(LC_ENCRYPTION_INFO_CMDSIZE)
  cmd, cmdsize, cryptoff, cryptsize, cryptid = lc.unpack("I5")

  if cmd != LC_ENCRYPTION_INFO_CMD or cmdsize != LC_ENCRYPTION_INFO_CMDSIZE
    STDERR.puts "Error: Invalid LC_ENCRYPTION_INFO structure at 0x%0.8x" % lc_offset
    exit 1
  elsif cryptsize != decrypt_dat.size
    STDERR.puts "Error: Your decrypted data from #{decrypt} does not have the correct size"
    STDERR.puts "Expected #{cryptsize} got #{decrypt_dat.size} from file"
    exit 1
  else
    STDERR.puts( "** Found LC_ENCRYPTION_INFO Structure at bytes offset #{lc_offset}",
                 "** lc_cmd=0x%0.8x, cmdsize=0x%0.8x, cryptoff=0x%0.8x, cryptsize=0x%0.8x, cryptid=0x%0.8x" %[
                   cmd, cmdsize, cryptoff, cryptsize, cryptid] )

    if cryptid != 0
      STDERR.puts "!! Patching cryptid"
      f.pos -= 4
      f.write("\x00\x00\x00\x00")
    end

    STDERR.puts "!! Writing #{cryptsize} bytes of decrypted data from #{decrypt.inspect} to #{exe.inspect}"
    # write our decrypted data at cryptoff, we assume it is equal to cryptsize
    f.pos = cryptoff
    f.write(decrypt_dat)
  end
end
