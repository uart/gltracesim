# coding=utf-8

"""
    stream.stream
    ~~~~~~~~~~~~~

    Implement the 'Stream' class and `open` method.

    :copyright: (c) 2016 by Ali Ghaffaari.
    :license: MIT, see LICENSE for more details.
"""

import sys
import gzip

from google.protobuf.internal.decoder import _DecodeVarint as varintDecoder
from google.protobuf.internal.encoder import _EncodeVarint as varintEncoder


def open(fpath, mode='rb',                   # pylint: disable=redefined-builtin
         buffer_size=0, **kwargs):
    """Open an stream."""
    return Stream(fpath, mode, buffer_size, **kwargs)


class Stream(object):
    """Stream class.

    Read and write protocol buffer streams encoded by 'stream' library. Stream
    objects instantiated for reading by setting mode to 'rb' (input `Stream`s)
    are iterable. So, protobuf objects can be obtained by iterate over the
    Stream. Stream iterator yields protobuf encoded data, so it should be parsed
    by using proper methods in Google Protocol Buffer library (for example
    `ParseFromString()` method).

    In output `Stream`s (those are instantiated with 'w' mode), method `write()`
    groups the given list of protobuf objects and writes them into the stream in
    the same format which is readable by any other parsers (refer to the stream
    library documentation for further information).

    The stream should be closed after performing all stream operations. Streams
    can be also used by `with` statement just like files.

    Attributes:
        _fd:            file object.
        _buffer_size:   size of the buffer to write as a one group of messages
                        (write-mode only).
        _write_buff:    list of buffered messages for writing (write-mode only).
        _group_delim:   if true it returns an instance of the `_delimiter` class
                        after reading each group of messages to indicate a group
                        change (read-mode only).
        _delimiter:     the delimiter class (read-mode only).
    """
    def __init__(self, fpath, mode='rb', buffer_size=0, gem5_format = True, **kwargs):
        """Constructor for the Stream class.

        Args:
            fpath (string): Path of the working file.
            mode (string): The mode argument can be any of 'r', 'rb', 'a', 'ab',
                'w', or 'wb', depending on whether the file will be read or
                written. The default is 'rb'.
            buffer_size (int): Write buffer size. The objects will be buffered
                before writing. No buffering will be made if buffer_size is 0.
                It means that size of the group will be determined by the size
                of object list provided on `write` call. Setting `buffer_size`
                to -1 means infinite buffer size. Method `flush` should be
                called manually or by closing stream. All objects will be write
                in one group upon `flush` or `close` events.

        Keyword args:
            group_delimiter (boolean): indicate the end of a message group if
                True by yielding a delimiter after reading each group.
            delimiter_cls (class): delimiter class.
        """
        self._fd = gzip.open(fpath, mode)

        # if not mode.startswith('r'):
        #     self._buffer_size = kwargs.pop('buffer_size', 0)
        #     self._write_buff = []
        # else:
        #     self._group_delim = kwargs.pop('group_delimiter', False)
        #     self._delimiter = kwargs.pop('delimiter_cls', None.__class__)

        self._buffer_size = buffer_size
        self._write_buff = []
        self._group_delim = kwargs.pop("group_delimiter", False)
        self._delimiter = kwargs.pop("delimiter_cls", None.__class__)

        self._gem5_format = gem5_format

        if gem5_format:
            magicNumber = b'\x67\x65\x6d\x35' #0x356d6567
            for m in magicNumber:
                assert m == self._fd.read(1)

    def __enter__(self):
        """Enter the runtime context related to Stream class. It will be
        automatically run by `with` statement.
        """
        return self

    def __exit__(self, *args):
        """Exit the runtime context related to Stream class. It will be
        automatically run by `with` statement. It closes the stream.
        """
        self.close()

    def __iter__(self):
        """Return the iterator object of the stream."""
        return self._get_objs()

    def _read_varint(self):
        """Read a varint from file, parse it, and return the decoded integer."""
        buff = self._fd.read(1)
        if buff == b'':
            return 0

        while (ord(buff[-1]) & 0x80) >> 7 == 1:  # while the MSB is 1
            new_byte = self._fd.read(1)
            if new_byte == b'':
                raise EOFError('unexpected EOF.')
            buff += new_byte

        varint, _ = varintDecoder(buff, 0)

        return varint

    def _get_objs(self):
        """A generator yielding all protobuf object data in the file. It is the
        main parser of the stream encoding.
        """
        more = True
        while more:

            if not self._gem5_format:
                count = self._read_varint()
                if count == 0:
                    break
                # Read a group containing `count` number of objects.
            else:
                count = sys.maxint

            for _ in xrange(count):
                size = self._read_varint()
                if size == 0:
                    if not self._gem5_format:
                        raise EOFError('unexpected EOF.')
                    else:
                        more = False
                        break
                # Read an object from the object group.
                try:
                    data = self._fd.read(size)
                except IOError, e:
                    # Mask IOErrors for now
                    print 'IOError Seen', e
                    return

                yield data

                if self._group_delim:
                    yield self._delimiter()

    def close(self):
        """Close the stream."""
        self.flush()
        self._fd.close()

    def write(self, *pb2_obj):
        """Write a group of one or more protobuf objects to the file. Multiple
        object groups can be written by calling this method several times before
        closing stream or exiting the runtime context.

        The input protobuf objects get buffered and will be written down when
        the number of buffered objects exceed the `self._buffer_size`.

        Args:
            pb2_obj (*protobuf.message.Message): list of protobuf messages.
        """
        base = len(self._write_buff)

        for idx, obj in enumerate(pb2_obj):
            if self._buffer_size > 0 and \
                    (idx + base) != 0 and \
                    (idx + base) % self._buffer_size == 0:
                self.flush()
            self._write_buff.append(obj)

        if self._buffer_size == 0:
            self.flush()

    def flush(self):
        """Write down buffer to the file."""
        count = len(self._write_buff)
        if count == 0:
            return

        if not self._gem5_format:
            varintEncoder(self._fd.write, count)

        for obj in self._write_buff:
            obj_str = obj.SerializeToString()
            varintEncoder(self._fd.write, len(obj_str))
            self._fd.write(obj_str)

        self._write_buff = []
