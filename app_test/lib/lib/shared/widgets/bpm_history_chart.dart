import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import '../../data/sensors/pulse/pulse_sample.dart';

class BpmHistoryChart extends StatelessWidget {
  final List<PulseSample> samples;

  const BpmHistoryChart({super.key, required this.samples});

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Container(
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(24),
        // Sombra suave difusa
        boxShadow: [
          BoxShadow(
            color: const Color(0xFF2B5C9E).withOpacity(0.08),
            blurRadius: 20,
            offset: const Offset(0, 10),
          ),
        ],
      ),
      padding: const EdgeInsets.all(20),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text('Frecuencia Cardíaca',
                      style: TextStyle(fontSize: 14, color: Colors.grey, fontWeight: FontWeight.w600)),
                  const SizedBox(height: 4),
                  Text(
                    samples.isNotEmpty ? '${samples.last.bpm.round()} BPM' : '--',
                    style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold, color: Color(0xFF2B5C9E)),
                  ),
                ],
              ),
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: Colors.red.withOpacity(0.1),
                  shape: BoxShape.circle,
                ),
                child: const Icon(Icons.favorite, color: Colors.red, size: 20),
              )
            ],
          ),
          const SizedBox(height: 24),
          SizedBox(
            height: 180,
            child: samples.isEmpty
                ? const Center(child: Text('Esperando datos...', style: TextStyle(color: Colors.grey)))
                : LineChart(_chartData(theme)),
          ),
        ],
      ),
    );
  }

  LineChartData _chartData(ThemeData theme) {
    // ... (Tu lógica de cálculo de spots se mantiene igual) ...
    final spots = samples.map((s) => FlSpot(s.timestampMs / 1000.0, s.bpm.toDouble())).toList();
    if (spots.isEmpty) return LineChartData();

    final double minX = spots.first.x;
    final double maxX = spots.last.x;

    return LineChartData(
      minX: minX,
      maxX: maxX,
      minY: 40,
      maxY: 180, // Ajuste fijo para estética
      gridData: FlGridData(
        show: true,
        drawVerticalLine: false,
        horizontalInterval: 40,
        getDrawingHorizontalLine: (value) => FlLine(
          color: Colors.grey.withOpacity(0.1),
          strokeWidth: 1,
          dashArray: [5, 5], // Línea punteada
        ),
      ),
      borderData: FlBorderData(show: false), // Sin bordes feos alrededor
      titlesData: FlTitlesData(
        topTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
        rightTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
        leftTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)), // Limpio, sin eje Y visible
        bottomTitles: AxisTitles(
          sideTitles: SideTitles(
            showTitles: true,
            interval: (maxX - minX) / 4, // Mostrar ~4 etiquetas
            getTitlesWidget: (value, meta) {
              final time = DateTime.fromMillisecondsSinceEpoch((value * 1000).toInt());
              return Padding(
                padding: const EdgeInsets.only(top: 8.0),
                child: Text(
                  DateFormat('HH:mm').format(time),
                  style: const TextStyle(color: Colors.grey, fontSize: 10),
                ),
              );
            },
          ),
        ),
      ),
      lineBarsData: [
        LineChartBarData(
          spots: spots,
          isCurved: true,
          curveSmoothness: 0.3,
          color: const Color(0xFFFA5A5A),
          barWidth: 3,
          isStrokeCapRound: true,
          dotData: const FlDotData(show: false),
          belowBarData: BarAreaData(
            show: true,
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: [
                const Color(0xFFFA5A5A).withOpacity(0.3),
                const Color(0xFFFA5A5A).withOpacity(0.0),
              ],
            ),
          ),
        ),
      ],
    );
  }
}