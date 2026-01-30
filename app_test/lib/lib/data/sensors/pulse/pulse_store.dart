import 'dart:async';

import 'pulse_sample.dart';
import 'pulse_state.dart';

class PulseStore {
  PulseStore._();
  static final PulseStore instance = PulseStore._();

  final StreamController<PulseState> _ctrl =
  StreamController<PulseState>.broadcast();

  PulseState _state = PulseState.initial();

  Stream<PulseState> get stream => _ctrl.stream;
  PulseState get state => _state;

  static const int _maxSamples = 2000;

  double _bpmOffset = 0.0;

  void setBpmOffset(double offset) {
    _bpmOffset = offset;
  }

  double get bpmOffset => _bpmOffset;

  void addSample(PulseSample sample) {
    final double? bpm = sample.bpm as double?;
    final double? correctedBpm = bpm == null ? null : (bpm + _bpmOffset);

    final history = List<PulseSample>.from(_state.history)..add(sample);

    if (history.length > _maxSamples) {
      history.removeAt(0);
    }

    _state = _state.copyWith(
      heartRate: correctedBpm?.round(),
      totalSamples: _state.totalSamples + 1,
      history: history,
    );

    _ctrl.add(_state);
  }
}
