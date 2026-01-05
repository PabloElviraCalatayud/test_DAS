import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'ota_protocol.dart';

class OtaSender {
  OtaSender(this._writeChar);

  final BluetoothCharacteristic _writeChar;

  static const int mtuPayload = 240;
  static const int headerSize = 1 + 2 + 2; // CMD + SEQ + LEN
  static const int chunkSize  = mtuPayload - headerSize;

  Future<void> sendFirmware(
      File bin,
      void Function(double progress) onProgress,
      ) async {
    final fw = await bin.readAsBytes();
    final total = fw.length;

    int seq = 0;
    int offset = 0;

    // --------------------
    // START
    // --------------------
    await _sendPacket(
      OtaPacket.build(
        cmd: OtaCmd.start,
        seq: 0,
      ),
    );

    // --------------------
    // DATA
    // --------------------
    while (offset < total) {
      final end = (offset + chunkSize < total)
          ? offset + chunkSize
          : total;

      final payload = fw.sublist(offset, end);

      await _sendPacket(
        OtaPacket.build(
          cmd: OtaCmd.data,
          seq: seq,
          payload: Uint8List.fromList(payload),
        ),
      );

      offset = end;
      seq++;

      onProgress(offset / total);
    }

    // --------------------
    // END
    // --------------------
    await _sendPacket(
      OtaPacket.build(
        cmd: OtaCmd.end,
        seq: 0,
      ),
    );
  }

  Future<void> _sendPacket(Uint8List pkt) async {
    await _writeChar.write(
      pkt,
      withoutResponse: true,
    );
  }
}
