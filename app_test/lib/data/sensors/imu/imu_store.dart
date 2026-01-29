import 'dart:async';

import 'imu_sample.dart';
import 'imu_state.dart';

class ImuStore {
  ImuStore._();
  static final ImuStore instance = ImuStore._();

  final StreamController<ImuState> _ctrl =
  StreamController.broadcast();

  ImuState _state = ImuState.initial();

  Stream<ImuState> get stream => _ctrl.stream;
  ImuState get state => _state;

  void addSample(ImuSample sample) {
    _state = _state.copyWith(
      lastSample: sample,
      totalSamples: _state.totalSamples + 1,
    );

    _ctrl.add(_state);
  }
}
