#!/usr/bin/env python

import sys, io
import packet_pb2 as packet
import job_pb2 as job
import stream

# Example from https://developers.google.com/protocol-buffers/docs/pythontutorial
if len(sys.argv) != 2:
    print "Usage:", sys.argv[0], "jobs.pb.gz"
    sys.exit(-1)

# Open the file and discard the header
istream = stream.open(sys.argv[1], "rb")

for msg in istream:
    hdr = packet.PacketHeader()
    hdr.ParseFromString(msg)
    print hdr
    #break

for msg in istream:
    jobinfo = job.JobInfo()
    jobinfo.ParseFromString(msg)
    print jobinfo

# Close the file
istream.close()

