import '../../../data/sensors/imu/imu_sample.dart';
import '../../../data/sensors/pulse/pulse_sample.dart';

class Packet {
  final int version;
  final int type;
  final int imuCount;
  final int pulseCount;
  final int timestampMs;

  final List<ImuSample> imuSamples;
  final List<PulseSample> pulseSamples;

  Packet({
    required this.version,
    required this.type,
    required this.imuCount,
    required this.pulseCount,
    required this.timestampMs,
    required this.imuSamples,
    required this.pulseSamples,
  });
}
