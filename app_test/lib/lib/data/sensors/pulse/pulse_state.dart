class PulseState {
  final int? heartRate;
  final int totalSamples;

  PulseState({
    required this.heartRate,
    required this.totalSamples,
  });

  factory PulseState.initial() {
    return PulseState(
      heartRate: null,
      totalSamples: 0,
    );
  }

  PulseState copyWith({
    int? heartRate,
    int? totalSamples,
  }) {
    return PulseState(
      heartRate: heartRate ?? this.heartRate,
      totalSamples: totalSamples ?? this.totalSamples,
    );
  }
}
