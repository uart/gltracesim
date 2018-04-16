#!/usr/bin/python

import sys, io
import packet_pb2 as packet
import frame_pb2 as frame
import stream

# Example from https://developers.google.com/protocol-buffers/docs/pythontutorial
if len(sys.argv) != 2:
	print "Usage:", sys.argv[0], "frames.pb.gz"
	sys.exit(-1)

# Open the file and discard the header
istream = stream.open(sys.argv[1], "rb")

for msg in istream:
    hdr = packet.PacketHeader()
    hdr.ParseFromString(msg)
    print hdr
    break

for msg in istream:
    frameinfo = frame.FrameInfo()
    frameinfo.ParseFromString(msg)
    print frameinfo

# Close the file
istream.close()
