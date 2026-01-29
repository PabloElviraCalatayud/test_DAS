import '../sensors/imu/imu_sample.dart';

class SleepScoreCalculator {
  double _movementAccum = 0;
  int _movementSamples = 0;

  double _bpmAccum = 0;
  int _bpmSamples = 0;

  void addImu(ImuSample s) {
    final m =
        s.ax.abs() + s.ay.abs() + s.az.abs();

    _movementAccum += m;
    _movementSamples++;
  }

  void addBpm(int bpm) {
    _bpmAccum += bpm;
    _bpmSamples++;
  }

  double get avgMovement =>
      _movementSamples == 0 ? 0 : _movementAccum / _movementSamples;

  double get avgBpm =>
      _bpmSamples == 0 ? 0 : _bpmAccum / _bpmSamples;

  int computeScore() {
    double score = 100;

    final movementPenalty = (avgMovement / 5000) * 40;
    score -= movementPenalty.clamp(0, 40);

    if (avgBpm > 0) {
      if (avgBpm < 50) score -= 10;
      if (avgBpm > 90) score -= 20;
    }

    return score.clamp(0, 100).round();
  }

  void reset() {
    _movementAccum = 0;
    _movementSamples = 0;
    _bpmAccum = 0;
    _bpmSamples = 0;
  }
}
