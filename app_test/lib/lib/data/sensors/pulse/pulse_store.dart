import 'dart:async';

import 'package:flutter_DAS_test/lib/data/sensors/pulse/pulse_sample.dart';
import 'package:flutter_DAS_test/lib/data/sensors/pulse/pulse_state.dart';

class PulseStore {
  PulseStore._();
  static final PulseStore instance = PulseStore._();

  final StreamController<PulseState> _ctrl =
  StreamController.broadcast();

  PulseState _state = PulseState.initial();

  Stream<PulseState> get stream => _ctrl.stream;
  PulseState get state => _state;

  void addSample(PulseSample sample) {
    final bpm = sample.raw > 0 ? (60000 / sample.raw) : null;

    _state = _state.copyWith(
      heartRate: bpm?.round(),
      totalSamples: _state.totalSamples + 1,
    );

    _ctrl.add(_state);
  }
}
