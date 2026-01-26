import 'package:flutter/material.dart';

import '../../data/health/health_state.dart';
import '../../data/health/health_store.dart';

class HealthPage extends StatelessWidget {
  const HealthPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Health',
          style: TextStyle(fontWeight: FontWeight.w700),
        ),
      ),
      body: StreamBuilder<HealthState>(
        stream: HealthStore.instance.stream,
        initialData: HealthStore.instance.state,
        builder: (context, snapshot) {
          final state = snapshot.data!;

          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                _sleepScoreCard(context, state),
                _pulseCard(context, state),
                _apneaSummaryCard(context, state),
                _apneaListCard(context, state),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _card(
      BuildContext context,
      String title,
      Widget child,
      ) {
    final scheme = Theme.of(context).colorScheme;

    return Container(
      margin: const EdgeInsets.only(bottom: 16),
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: scheme.surface,
        borderRadius: BorderRadius.circular(16),
        boxShadow: [
          BoxShadow(
            blurRadius: 10,
            offset: const Offset(0, 4),
            color: scheme.onSurface.withOpacity(0.08),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            title,
            style: Theme.of(context).textTheme.titleLarge?.copyWith(
              fontWeight: FontWeight.w700,
            ),
          ),
          const SizedBox(height: 12),
          child,
        ],
      ),
    );
  }

  Widget _sleepScoreCard(BuildContext context, HealthState state) {
    final score = state.sleepScore;

    String label;
    Color color;

    if (score >= 85) {
      label = 'Excelente';
      color = Colors.green;
    } else if (score >= 65) {
      label = 'Aceptable';
      color = Colors.orange;
    } else {
      label = 'Deficiente';
      color = Colors.red;
    }

    return _card(
      context,
      'Sleep Score',
      Row(
        children: [
          Text(
            '$score',
            style: TextStyle(
              fontSize: 64,
              fontWeight: FontWeight.w800,
              color: color,
            ),
          ),
          const SizedBox(width: 16),
          Text(
            label,
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.w600,
              color: color,
            ),
          ),
        ],
      ),
    );
  }

  Widget _pulseCard(BuildContext context, HealthState state) {
    final bpm = state.avgBpm;

    return _card(
      context,
      'Pulso cardíaco',
      Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            bpm > 0 ? '${bpm.toStringAsFixed(1)} BPM' : '-',
            style: const TextStyle(
              fontSize: 28,
              fontWeight: FontWeight.w700,
            ),
          ),
          Icon(
            Icons.favorite,
            color: bpm > 90 ? Colors.red : Colors.pink,
            size: 32,
          ),
        ],
      ),
    );
  }

  Widget _apneaSummaryCard(BuildContext context, HealthState state) {
    return _card(
      context,
      'Apneas detectadas',
      Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            '${state.apneaCount} eventos',
            style: const TextStyle(
              fontSize: 22,
              fontWeight: FontWeight.w700,
            ),
          ),
          const SizedBox(height: 8),
          LinearProgressIndicator(
            value: state.apneaProbability,
            minHeight: 8,
            backgroundColor: Colors.grey.shade300,
            color: state.apneaProbability > 0.5
                ? Colors.red
                : Colors.orange,
          ),
          const SizedBox(height: 6),
          Text(
            'Probabilidad estimada: ${(state.apneaProbability * 100).round()}%',
            style: const TextStyle(fontWeight: FontWeight.w500),
          ),
        ],
      ),
    );
  }

  Widget _apneaListCard(BuildContext context, HealthState state) {
    if (state.apneas.isEmpty) {
      return _card(
        context,
        'Eventos recientes',
        const Text('No se han detectado eventos'),
      );
    }

    return _card(
      context,
      'Eventos recientes',
      Column(
        children: state.apneas.reversed.take(5).map((e) {
          return Padding(
            padding: const EdgeInsets.symmetric(vertical: 6),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  '${e.duration.inSeconds}s',
                  style: const TextStyle(fontWeight: FontWeight.w600),
                ),
                Text(
                  '${e.minBpm.toStringAsFixed(0)} → ${e.maxBpm.toStringAsFixed(0)} BPM',
                  style: const TextStyle(fontWeight: FontWeight.w500),
                ),
              ],
            ),
          );
        }).toList(),
      ),
    );
  }
}
