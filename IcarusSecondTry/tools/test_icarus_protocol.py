import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import icarus_protocol as protocol


class ProtocolTest(unittest.TestCase):
    def test_round_trip_and_crc(self):
        packet = protocol.Packet(protocol.ECHO_REQUEST, 0x10203040,
                                 0x50607080, b"\x10\x20\x30", 0x5A)
        encoded = protocol.encode_packet(packet)
        self.assertEqual(encoded.hex(),
                         "49430105145a00031020304050607080deddf854102030")
        self.assertEqual(protocol.decode_packet(encoded), packet)
        with self.assertRaises(protocol.ProtocolError):
            protocol.decode_packet(encoded[:-1])
        with self.assertRaises(protocol.ProtocolError):
            protocol.decode_packet(encoded[:20] + b"\x11\x20\x30")

    def test_command_encoding(self):
        self.assertEqual(protocol.encode_command(7, protocol.SET_MOTOR_RPM,
                                                 motor_rpm=-1200).hex(),
                         "000000070304fffffb50")
        self.assertEqual(protocol.encode_command(8, protocol.SET_SOLENOID,
                                                 solenoid_index=23,
                                                 solenoid_open=True).hex(),
                         "0000000804021701")


if __name__ == "__main__":
    unittest.main()
