class PulseBpmCalculator {
  double _threshold = 2000.0;
  bool _pulseDetected = false;
  int _lastPulseTimeUs = 0;
  double _lastBpm = 0.0;

  double process(int raw, int timestampMs) {
    final int nowUs = timestampMs * 1000;

    _threshold = 0.95 * _threshold + 0.05 * raw;

    if (!_pulseDetected && raw > _threshold + 250) {
      _pulseDetected = true;

      if (_lastPulseTimeUs > 0) {
        final double intervalS =
            (nowUs - _lastPulseTimeUs) / 1000000.0;

        final double bpm = 60.0 / intervalS;
        _lastBpm = 0.8 * _lastBpm + 0.2 * bpm;
      }

      _lastPulseTimeUs = nowUs;
    }

    if (_pulseDetected && raw < _threshold) {
      _pulseDetected = false;
    }

    return _lastBpm;
  }

  double get lastBpm => _lastBpm;
}
