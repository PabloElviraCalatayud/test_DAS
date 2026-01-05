import 'imu_sample.dart';

class ImuState {
  final ImuSample? lastSample;
  final int totalSamples;

  ImuState({
    required this.lastSample,
    required this.totalSamples,
  });

  factory ImuState.initial() {
    return ImuState(
      lastSample: null,
      totalSamples: 0,
    );
  }

  ImuState copyWith({
    ImuSample? lastSample,
    int? totalSamples,
  }) {
    return ImuState(
      lastSample: lastSample ?? this.lastSample,
      totalSamples: totalSamples ?? this.totalSamples,
    );
  }
}
