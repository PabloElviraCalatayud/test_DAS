import 'dart:async';
import 'package:flutter/material.dart';

import '../../data/health/health_store.dart';
import '../../data/health/health_state.dart';
import '../../data/sensors/pulse/pulse_store.dart';
import '../../data/sensors/pulse/pulse_sample.dart';

import '../../shared/widgets/bpm_history_chart.dart';
import '../../shared/widgets/health_card.dart';
import '../../shared/widgets/sleep_score_card.dart';

class HealthContent extends StatefulWidget {
  final HealthStore healthStore;
  final PulseStore pulseStore;

  const HealthContent({
    super.key,
    required this.healthStore,
    required this.pulseStore,
  });

  @override
  State<HealthContent> createState() => _HealthContentState();
}

class _HealthContentState extends State<HealthContent> {
  StreamSubscription? _healthSub;
  final List<PulseSample> _history = [];

  @override
  void initState() {
    super.initState();

    _healthSub = widget.healthStore.stream.listen((_) {
      final avgBpm = widget.healthStore.state.avgBpm;

      if (avgBpm > 0) {
        _history.add(
          PulseSample(
            raw: 0,
            timestampMs: DateTime.now().millisecondsSinceEpoch,
            bpm: avgBpm,
          ),
        );
      }

      if (mounted) setState(() {});
    });
  }

  @override
  void dispose() {
    _healthSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final HealthState healthState = widget.healthStore.state;

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        HealthCard(
          title: 'Pulso cardíaco',
          value: healthState.avgBpm == 0
              ? '—'
              : '${healthState.avgBpm.round()} BPM',
          subtitle: _pulseLabel(healthState.avgBpm),
          icon: Icons.favorite,
        ),
        const SizedBox(height: 16),

        HealthCard(
          title: 'Eventos de apnea',
          value: '${healthState.apneaCount}',
          subtitle: _apneaLabel(healthState.apneaCount),
          icon: Icons.air,
        ),
        const SizedBox(height: 16),

        BpmHistoryChart(
          samples: _history,
        ),
        const SizedBox(height: 16),

        SleepScoreCard(
          score: healthState.sleepScore,
          movementIndex: healthState.movementIndex,
        ),
      ],
    );
  }

  String _pulseLabel(double bpm) {
    if (bpm == 0) return 'Sin datos';
    if (bpm < 50) return 'Bajo';
    if (bpm <= 100) return 'Normal';
    return 'Alto';
  }

  String _apneaLabel(int count) {
    if (count == 0) return 'Sin eventos';
    if (count < 3) return 'Leve';
    if (count < 6) return 'Moderado';
    return 'Severo';
  }
}
