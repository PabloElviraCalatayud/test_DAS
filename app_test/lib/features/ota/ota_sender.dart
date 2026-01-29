import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'ota_protocol.dart';

class OtaSender {
  OtaSender(this._writeChar, this._notifyChar) {
    _instance = this;
  }

  final BluetoothCharacteristic _writeChar;
  final BluetoothCharacteristic _notifyChar;

  static OtaSender? _instance;
  Completer<void>? _ackCompleter;

  static const int mtuPayload = 240;
  static const int headerSize = 5;
  static const int chunkSize = mtuPayload - headerSize;

  static void handleNotify(Uint8List data) {
    if (data.length < 5) {
      return;
    }

    final pkt = OtaPacket.parse(data);

    if (pkt.cmd == OtaCmd.ack) {
      _instance?._ackCompleter?.complete();
    }

    if (pkt.cmd == OtaCmd.nack) {
      _instance?._ackCompleter
          ?.completeError(Exception('OTA NACK'));
    }
  }

  Future<void> sendFirmware(
      File bin,
      void Function(double progress) onProgress,
      ) async {
    final fw = await bin.readAsBytes();
    final total = fw.length;

    int seq = 0;
    int offset = 0;

    await _sendAndWaitAck(
      OtaPacket.build(cmd: OtaCmd.start, seq: 0),
    );

    while (offset < total) {
      final end = (offset + chunkSize < total)
          ? offset + chunkSize
          : total;

      final payload = fw.sublist(offset, end);

      await _sendAndWaitAck(
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

    await _sendAndWaitAck(
      OtaPacket.build(cmd: OtaCmd.end, seq: 0),
    );
  }

  Future<void> _sendAndWaitAck(Uint8List pkt) async {
    final completer = Completer<void>();
    _ackCompleter = completer;

    await _writeChar.write(pkt, withoutResponse: true);

    await completer.future.timeout(
      const Duration(seconds: 2),
      onTimeout: () => throw Exception('OTA ACK timeout'),
    );
  }
}
