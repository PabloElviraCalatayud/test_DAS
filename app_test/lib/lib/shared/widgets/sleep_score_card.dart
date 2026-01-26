import 'package:flutter/material.dart';

class SleepScoreCard extends StatelessWidget {
  final int? score;
  final double movementIndex;

  const SleepScoreCard({
    super.key,
    required this.score,
    required this.movementIndex,
  });

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final int value = score ?? 0;

    return Container(
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
          const Text(
            'Sleep Score',
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 12),
          Center(
            child: Text(
              score == null ? '—' : value.toString(),
              style: TextStyle(
                fontSize: 48,
                fontWeight: FontWeight.bold,
                color: _scoreColor(scheme, value),
              ),
            ),
          ),
          const SizedBox(height: 8),
          LinearProgressIndicator(
            value: score == null ? null : value / 100,
            minHeight: 10,
            backgroundColor: scheme.surfaceVariant,
            color: _scoreColor(scheme, value),
          ),
          const SizedBox(height: 12),
          Text(
            _scoreLabel(value),
            style: Theme.of(context).textTheme.bodyMedium,
          ),
          const SizedBox(height: 4),
          Text(
            'Índice de movimiento: ${movementIndex.toStringAsFixed(1)}',
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ],
      ),
    );
  }

  Color _scoreColor(ColorScheme scheme, int score) {
    if (score >= 80) return Colors.green;
    if (score >= 50) return Colors.orange;
    return Colors.red;
  }

  String _scoreLabel(int score) {
    if (score >= 80) return 'Sueño reparador';
    if (score >= 50) return 'Sueño aceptable';
    return 'Sueño deficiente';
  }
}
