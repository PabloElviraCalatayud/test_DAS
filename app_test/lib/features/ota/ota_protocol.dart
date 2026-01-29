import 'dart:typed_data';

enum OtaCmd {
  start(0x01),
  data(0x02),
  end(0x03),
  ack(0x80),
  nack(0x81);

  const OtaCmd(this.value);
  final int value;

  static OtaCmd fromValue(int v) {
    return OtaCmd.values.firstWhere(
          (e) => e.value == v,
      orElse: () => throw Exception('Unknown OtaCmd 0x${v.toRadixString(16)}'),
    );
  }
}

class OtaPacket {
  OtaPacket({
    required this.cmd,
    required this.seq,
    this.payload,
  });

  final OtaCmd cmd;
  final int seq;
  final Uint8List? payload;

  static Uint8List build({
    required OtaCmd cmd,
    required int seq,
    Uint8List? payload,
  }) {
    final payloadLen = payload?.length ?? 0;
    final buf = Uint8List(5 + payloadLen);
    final bd = ByteData.view(buf.buffer);

    bd.setUint8(0, cmd.value);
    bd.setUint16(1, seq, Endian.little);
    bd.setUint16(3, payloadLen, Endian.little);

    if (payloadLen > 0) {
      buf.setRange(5, 5 + payloadLen, payload!);
    }

    return buf;
  }

  static OtaPacket parse(Uint8List data) {
    if (data.length < 5) {
      throw Exception('OTA packet too short (${data.length})');
    }

    final bd = ByteData.view(data.buffer);
    final cmd = OtaCmd.fromValue(bd.getUint8(0));
    final seq = bd.getUint16(1, Endian.little);
    final len = bd.getUint16(3, Endian.little);

    Uint8List? payload;
    if (len > 0) {
      payload = data.sublist(5, 5 + len);
    }

    return OtaPacket(cmd: cmd, seq: seq, payload: payload);
  }
}
