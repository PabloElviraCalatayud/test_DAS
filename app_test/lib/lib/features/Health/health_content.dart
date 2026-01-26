import 'dart:async';
import 'package:flutter/material.dart';

import '../../data/health/health_store.dart';
import '../../data/health/health_state.dart';
import '../../shared/widgets/health_card.dart';
import '../../shared/widgets/sleep_score_card.dart';


class HealthContent extends StatefulWidget {
  final HealthStore healthStore;

  const HealthContent({
    super.key,
    required this.healthStore,
  });

  @override
  State<HealthContent> createState() => _HealthContentState();
}

class _HealthContentState extends State<HealthContent> {
  StreamSubscription? _sub;

  @override
  void initState() {
    super.initState();
    _sub = widget.healthStore.stream.listen((_) {
      if (mounted) setState(() {});
    });
  }

  @override
  void dispose() {
    _sub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final HealthState state = widget.healthStore.state;

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        HealthCard(
          title: 'Pulso cardíaco',
          value: state.avgBpm == 0
              ? '—'
              : '${state.avgBpm.round()} BPM',
          subtitle: _pulseLabel(state.avgBpm),
          icon: Icons.favorite,
        ),
        const SizedBox(height: 16),
        SleepScoreCard(
          score: state.sleepScore,
          movementIndex: state.movementIndex,
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
}
