import 'dart:async';

import '../../features/packet/logic/packet_decoder.dart';
import '../sensors/imu/imu_sample.dart';
import '../sensors/imu/imu_state.dart';
import '../sensors/imu/imu_store.dart';
import 'apnea_event.dart';
import 'health_state.dart';

enum _ApneaState {
  idle,
  detecting,
}

class HealthStore {
  HealthStore._internal() {
    PacketDecoder.instance.stream.listen(_onPacket);
    ImuStore.instance.stream.listen(_onImuState);
  }

  static final HealthStore instance = HealthStore._internal();

  final HealthState _state = HealthState();
  HealthState get state => _state;

  final StreamController<HealthState> _ctrl =
  StreamController.broadcast();

  Stream<HealthState> get stream => _ctrl.stream;

  ImuSample? _lastImu;

  _ApneaState _apneaState = _ApneaState.idle;

  DateTime? _apneaStart;
  double _baselineBpm = 0;
  double _minBpm = double.infinity;
  double _maxBpm = 0;

  final List<double> _bpmWindow = [];
  static const int _baselineWindowSize = 20;

  static const double _bpmDropAbsolute = 5.0;
  static const double _movementLowThreshold = 2.0;

  static const Duration _minApneaDuration =
  Duration(seconds: 1);
  static const Duration _maxApneaDuration =
  Duration(seconds: 30);

  void _onPacket(pkt) {
    if (pkt.pulseSamples.isEmpty) {
      return;
    }

    final bpm = pkt.pulseSamples.last.bpm;
    if (bpm == null || bpm <= 0) {
      return;
    }

    _state.avgBpm = bpm;
    _updateBaseline(bpm);
    _detectApnea();
    _recalculateSleepScore();
    _ctrl.add(_state);
  }

  void _onImuState(ImuState imuState) {
    final s = imuState.lastSample;
    if (s == null) {
      return;
    }

    if (_lastImu != null) {
      final dx = (s.axMs2 - _lastImu!.axMs2).abs();
      final dy = (s.ayMs2 - _lastImu!.ayMs2).abs();
      final dz = (s.azMs2 - _lastImu!.azMs2).abs();

      final movement = dx + dy + dz;

      _state.movementIndex =
          (_state.movementIndex * 0.8) + (movement * 0.2);

      _detectApnea();
      _recalculateSleepScore();
      _ctrl.add(_state);
    }

    _lastImu = s;
  }

  void _updateBaseline(double bpm) {
    _bpmWindow.add(bpm);
    if (_bpmWindow.length > _baselineWindowSize) {
      _bpmWindow.removeAt(0);
    }

    _baselineBpm =
        _bpmWindow.reduce((a, b) => a + b) /
            _bpmWindow.length;
  }

  void _detectApnea() {
    if (_baselineBpm == 0) {
      return;
    }

    final now = DateTime.now();
    final bpm = _state.avgBpm;

    final lowMovement =
        _state.movementIndex < _movementLowThreshold;

    final bpmDrop =
        (_baselineBpm - bpm) >= _bpmDropAbsolute;

    final apneaCondition = bpmDrop || lowMovement;

    switch (_apneaState) {
      case _ApneaState.idle:
        if (apneaCondition) {
          _apneaState = _ApneaState.detecting;
          _apneaStart = now;
          _minBpm = bpm;
          _maxBpm = bpm;
        }
        break;

      case _ApneaState.detecting:
        _minBpm = bpm < _minBpm ? bpm : _minBpm;
        _maxBpm = bpm > _maxBpm ? bpm : _maxBpm;

        final duration =
        now.difference(_apneaStart!);

        if (!apneaCondition) {
          if (duration >= _minApneaDuration &&
              duration <= _maxApneaDuration) {
            _registerApnea(now);
          }
          _resetApnea();
        }
        break;
    }
  }

  void _registerApnea(DateTime end) {
    final event = ApneaEvent(
      start: _apneaStart!,
      end: end,
      minBpm: _minBpm,
      maxBpm: _maxBpm,
    );

    _state.apneas.add(event);
    _state.apneaCount = _state.apneas.length;
    _state.apneaProbability =
        (_state.apneaCount / 5).clamp(0, 1);
  }

  void _resetApnea() {
    _apneaState = _ApneaState.idle;
    _apneaStart = null;
    _minBpm = double.infinity;
    _maxBpm = 0;
  }

  void _recalculateSleepScore() {
    double score = 100;

    if (_state.avgBpm > 80) score -= 15;
    if (_state.avgBpm > 100) score -= 25;

    if (_state.movementIndex > 5) score -= 15;
    if (_state.movementIndex > 10) score -= 30;

    if (_state.apneaCount > 0) score -= 10;
    if (_state.apneaCount > 3) score -= 25;

    _state.sleepScore = score.clamp(0, 100).round();
  }
}
