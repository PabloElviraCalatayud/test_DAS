import 'dart:typed_data';
import '../../../data/sensors/imu/imu_sample.dart';
import '../../../data/sensors/pulse/pulse_sample.dart';


class PacketDebug {
  final int version;
  final int type;
  final int imuCount;
  final int pulseCount;
  final int timestampMs;
  final List<ImuSample> imuSamples;
  final List<PulseSample> pulseSamples;
  final Uint8List raw;

  PacketDebug({
    required this.version,
    required this.type,
    required this.imuCount,
    required this.pulseCount,
    required this.timestampMs,
    required this.imuSamples,
    required this.pulseSamples,
    required this.raw,
  });
}
