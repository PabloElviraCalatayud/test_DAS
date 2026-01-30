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
    final theme = Theme.of(context);
    final scheme = theme.colorScheme;

    final bool hasScore = score != null;
    final int value = score ?? 0;

    final color = hasScore ? _scoreColor(value) : scheme.outlineVariant;

    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            color.withOpacity(0.10),
            scheme.surface, // en vez de Colors.white
          ],
        ),
        borderRadius: BorderRadius.circular(24),
        border: Border.all(color: color.withOpacity(0.20)),
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Text(
                'Calidad del Sueño',
                style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
              ),
              Container(
                padding:
                const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                decoration: BoxDecoration(
                  color: color.withOpacity(0.20),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: Text(
                  hasScore ? _scoreLabel(value) : 'Sin datos',
                  style: TextStyle(
                    color: color,
                    fontWeight: FontWeight.bold,
                    fontSize: 12,
                  ),
                ),
              )
            ],
          ),
          const SizedBox(height: 20),

          Stack(
            alignment: Alignment.center,
            children: [
              SizedBox(
                height: 100,
                width: 100,
                child: CircularProgressIndicator(
                  value: hasScore ? (value / 100) : 0, // null -> 0
                  backgroundColor: scheme.surfaceVariant,
                  color: color,
                  strokeWidth: 10,
                  strokeCap: StrokeCap.round,
                ),
              ),
              Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    hasScore ? value.toString() : '—',
                    style: TextStyle(
                      fontSize: 32,
                      fontWeight: FontWeight.w800,
                      color: scheme.onSurface,
                    ),
                  ),
                  const Text(
                    'puntos',
                    style: TextStyle(fontSize: 10, color: Colors.grey),
                  ),
                ],
              )
            ],
          ),

          const SizedBox(height: 20),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.directions_run, size: 16, color: Colors.grey[600]),
              const SizedBox(width: 4),
              Text(
                'Movimiento: ${movementIndex.toStringAsFixed(1)}',
                style: TextStyle(color: Colors.grey[600], fontSize: 13),
              ),
            ],
          )
        ],
      ),
    );
  }

  Color _scoreColor(int score) {
    if (score >= 80) return const Color(0xFF00BFA5);
    if (score >= 50) return const Color(0xFFFFA000);
    return const Color(0xFFFF5252);
  }

  String _scoreLabel(int score) {
    if (score >= 80) return 'Óptimo';
    if (score >= 50) return 'Regular';
    return 'Atención';
  }
}
