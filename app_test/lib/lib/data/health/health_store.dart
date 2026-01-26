import 'dart:async';

import '../../features/packet/logic/packet_decoder.dart';
import '../sensors/imu/imu_sample.dart';
import '../sensors/imu/imu_state.dart';
import '../sensors/imu/imu_store.dart';
import 'apnea_event.dart';
import 'health_state.dart';

enum _ApneaState {
  idle,
  possible,
  confirmed,
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
  static const int _baselineWindowSize = 30;

  void _onPacket(pkt) {
    if (pkt.pulseSamples.isNotEmpty) {
      final bpm = pkt.pulseSamples.last.bpm;
      if (bpm != null) {
        _state.avgBpm = bpm;
        _updateBaseline(bpm);
        _detectApnea();
        _recalculateSleepScore();
        _ctrl.add(_state);
      }
    }
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
          (_state.movementIndex * 0.9) + (movement * 0.1);

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
        _bpmWindow.reduce((a, b) => a + b) / _bpmWindow.length;
  }

  void _detectApnea() {
    if (_baselineBpm == 0) {
      return;
    }

    final now = DateTime.now();
    final bpm = _state.avgBpm;

    final immobile = _state.movementIndex < 0.15;
    final bpmDrop = bpm < _baselineBpm * 0.95;
    final bpmRebound = bpm > _baselineBpm * 1.08;

    switch (_apneaState) {
      case _ApneaState.idle:
        if (immobile) {
          _apneaState = _ApneaState.possible;
          _apneaStart = now;
          _minBpm = bpm;
          _maxBpm = bpm;
        }
        break;

      case _ApneaState.possible:
        _minBpm = bpm < _minBpm ? bpm : _minBpm;
        _maxBpm = bpm > _maxBpm ? bpm : _maxBpm;

        if (bpmDrop &&
            now.difference(_apneaStart!).inSeconds >= 10) {
          _apneaState = _ApneaState.confirmed;
        }

        if (!immobile) {
          _resetApnea();
        }
        break;

      case _ApneaState.confirmed:
        _minBpm = bpm < _minBpm ? bpm : _minBpm;
        _maxBpm = bpm > _maxBpm ? bpm : _maxBpm;

        if (bpmRebound || !immobile) {
          final duration =
              now.difference(_apneaStart!).inSeconds;

          if (duration >= 10 && duration <= 60) {
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
        (_state.apneaCount / 8).clamp(0, 1);
  }

  void _resetApnea() {
    _apneaState = _ApneaState.idle;
    _apneaStart = null;
    _minBpm = double.infinity;
    _maxBpm = 0;
  }

  void _recalculateSleepScore() {
    double score = 100;

    if (_state.avgBpm > 80) score -= 20;
    if (_state.avgBpm > 90) score -= 20;

    if (_state.movementIndex > 2.0) score -= 15;
    if (_state.movementIndex > 4.0) score -= 25;
    if (_state.movementIndex > 6.0) score -= 40;

    if (_state.apneaCount > 2) score -= 10;
    if (_state.apneaCount > 5) score -= 25;

    _state.sleepScore = score.clamp(0, 100).round();
  }
}
