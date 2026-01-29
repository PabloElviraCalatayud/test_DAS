import 'pulse_sample.dart';

class PulseState {
  final int? heartRate;
  final int totalSamples;
  final List<PulseSample> history;

  PulseState({
    required this.heartRate,
    required this.totalSamples,
    required this.history,
  });

  factory PulseState.initial() {
    return PulseState(
      heartRate: null,
      totalSamples: 0,
      history: const [],
    );
  }

  PulseState copyWith({
    int? heartRate,
    int? totalSamples,
    List<PulseSample>? history,
  }) {
    return PulseState(
      heartRate: heartRate ?? this.heartRate,
      totalSamples: totalSamples ?? this.totalSamples,
      history: history ?? this.history,
    );
  }
}
