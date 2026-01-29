import 'package:flutter/material.dart';

class ApneaStatsCard extends StatelessWidget {
  final int totalEvents;
  final Duration sessionDuration; // Necesario para calcular eventos/hora

  const ApneaStatsCard({
    super.key,
    required this.totalEvents,
    required this.sessionDuration,
  });

  @override
  Widget build(BuildContext context) {
    // Cálculo seguro de eventos por hora (AHI)
    double hours = sessionDuration.inMinutes / 60.0;
    double ahi = (hours > 0) ? totalEvents / hours : 0.0;

    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(24),
        boxShadow: [
          BoxShadow(
            color: const Color(0xFF2B5C9E).withOpacity(0.08),
            blurRadius: 20,
            offset: const Offset(0, 10),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Eventos Respiratorios',
            style: TextStyle(
              fontSize: 16,
              fontWeight: FontWeight.bold,
              color: Colors.black87,
            ),
          ),
          const SizedBox(height: 20),
          Row(
            children: [
              // COLUMNA 1: TOTALES
              Expanded(
                child: _StatItem(
                  label: 'Total Eventos',
                  value: totalEvents.toString(),
                  color: const Color(0xFFFF5252), // Rojo alerta
                  icon: Icons.warning_amber_rounded,
                ),
              ),
              Container(width: 1, height: 50, color: Colors.grey.withOpacity(0.2)),
              // COLUMNA 2: POR HORA (AHI)
              Expanded(
                child: _StatItem(
                  label: 'Eventos / Hora',
                  value: ahi.toStringAsFixed(1),
                  color: const Color(0xFFFFA000), // Naranja diagnóstico
                  icon: Icons.speed_rounded,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _StatItem extends StatelessWidget {
  final String label;
  final String value;
  final Color color;
  final IconData icon;

  const _StatItem({
    required this.label,
    required this.value,
    required this.color,
    required this.icon,
  });

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Icon(icon, color: color, size: 28),
        const SizedBox(height: 8),
        Text(
          value,
          style: TextStyle(
            fontSize: 24,
            fontWeight: FontWeight.w800,
            color: color,
          ),
        ),
        const SizedBox(height: 4),
        Text(
          label,
          style: TextStyle(
            fontSize: 12,
            color: Colors.grey[600],
            fontWeight: FontWeight.w500,
          ),
        ),
      ],
    );
  }
}