#!/usr/bin/env ruby
#
#


if (outfile=ARGV.shift).nil?
  $stderr.puts "Usage: #{File.basename(__FILE__)} outfile.xcent [..entitlements one arg at a time..]"
  exit 1
end

entitlements = <<-_EOF_
<plist version="1.0"><dict><key>keychain-access-groups</key><array>
_EOF_

ARGV.each do |ent|
  entitlements << "  <string>#{ent}</string>\n"
end

entitlements << "</array></dict></plist>"

File.open(outfile, 'w') do |f|
  f.write("\xfa\xde\x71\x71")
  f.write([entitlements.size + 8].pack("N"))
  f.write(entitlements)
end
