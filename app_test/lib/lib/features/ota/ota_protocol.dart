import 'dart:typed_data';

class OtaCmd {
  static const int start = 0x01;
  static const int data  = 0x02;
  static const int end   = 0x03;
  static const int abort = 0x04;
}

class OtaPacket {
  static Uint8List build({
    required int cmd,
    required int seq,
    Uint8List? payload,
  }) {
    payload ??= Uint8List(0);

    final len = payload.length;
    final buffer = BytesBuilder();

    buffer.addByte(cmd);

    final bd = ByteData(4);
    bd.setUint16(0, seq, Endian.little);
    bd.setUint16(2, len, Endian.little);

    buffer.add(bd.buffer.asUint8List());
    buffer.add(payload);

    return buffer.toBytes();
  }
}
