import 'dart:async';
import 'dart:typed_data';

import '../../../data/sensors/imu/imu_sample.dart';
import '../../../data/sensors/imu/imu_store.dart';
import '../../../data/sensors/pulse/pulse_bpm_calculator.dart';
import '../../../data/sensors/pulse/pulse_sample.dart';
import '../../../data/sensors/pulse/pulse_store.dart';
import '../data/packet_debug.dart';

class PacketDecoder {
  PacketDecoder._();
  static final PacketDecoder instance = PacketDecoder._();

  static const int packetSize = 88;

  static const int imuMax = 6;
  static const int pulseMax = 4;

  static const int imuSize = 12;
  static const int pulseSize = 2;

  static const int headerSize = 8;
  static const int imuBlockSize = imuMax * imuSize;
  final PulseBpmCalculator _pulseBpmCalc = PulseBpmCalculator();

  final StreamController<PacketDebug> _ctrl =
  StreamController.broadcast();

  Stream<PacketDebug> get stream => _ctrl.stream;

  final BytesBuilder _buffer = BytesBuilder();

  void decode(Uint8List chunk) {
    _buffer.add(chunk);

    while (_buffer.length >= packetSize) {
      final bytes = _buffer.takeBytes();

      final packet = Uint8List.fromList(
        bytes.sublist(0, packetSize),
      );

      if (bytes.length > packetSize) {
        _buffer.add(bytes.sublist(packetSize));
      }

      _decodePacket(packet);
    }
  }

  void _decodePacket(Uint8List data) {
    final b = ByteData.sublistView(data);

    final version = b.getUint8(0);
    final type = b.getUint8(1);
    final imuCount = b.getUint8(2);
    final pulseCount = b.getUint8(3);
    final ts = b.getUint32(4, Endian.little);

    final imuSamples = <ImuSample>[];
    final pulseSamples = <PulseSample>[];

    int offset = headerSize;

    // -----------------------------
    // IMU (bloque fijo)
    // -----------------------------
    for (int i = 0; i < imuCount && i < imuMax; i++) {
      final ax = b.getInt16(offset + 0, Endian.little);
      final ay = b.getInt16(offset + 2, Endian.little);
      final az = b.getInt16(offset + 4, Endian.little);
      final gx = b.getInt16(offset + 6, Endian.little);
      final gy = b.getInt16(offset + 8, Endian.little);
      final gz = b.getInt16(offset + 10, Endian.little);

      final sample = ImuSample(
        ax: ax,
        ay: ay,
        az: az,
        gx: gx,
        gy: gy,
        gz: gz,
        timestampMs: ts,
      );

      imuSamples.add(sample);
      ImuStore.instance.addSample(sample);

      offset += imuSize;
    }

    // Saltar IMUs no usados
    offset = headerSize + imuBlockSize;

    // -----------------------------
    // PULSE (bloque fijo)
    // -----------------------------
    for (int i = 0; i < pulseCount && i < pulseMax; i++) {
      final raw = b.getUint16(offset, Endian.little);

      final bpm = _pulseBpmCalc.process(raw, ts);

      final sample = PulseSample(
        raw: raw,
        timestampMs: ts,
        bpm: bpm,
      );

      pulseSamples.add(sample);
      PulseStore.instance.addSample(sample);

      offset += pulseSize;
    }

    _ctrl.add(
      PacketDebug(
        version: version,
        type: type,
        imuCount: imuCount,
        pulseCount: pulseCount,
        timestampMs: ts,
        imuSamples: imuSamples,
        pulseSamples: pulseSamples,
        raw: data,
      ),
    );
  }
}
