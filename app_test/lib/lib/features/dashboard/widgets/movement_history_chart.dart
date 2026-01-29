import 'dart:math';
import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import '../../../data/sensors/imu/imu_sample.dart';

class MovementHistoryChart extends StatelessWidget {
  final List<ImuSample> samples;

  const MovementHistoryChart({super.key, required this.samples});

  @override
  Widget build(BuildContext context) {
    return Container(
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
      padding: const EdgeInsets.all(20),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Mapa de Movimiento',
                      style: TextStyle(fontSize: 14, color: Colors.grey, fontWeight: FontWeight.w600)),
                  SizedBox(height: 4),
                  Text(
                    'Actividad Física',
                    style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Color(0xFF673AB7)),
                  ),
                ],
              ),
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: const Color(0xFF673AB7).withOpacity(0.1),
                  shape: BoxShape.circle,
                ),
                child: const Icon(Icons.directions_run_rounded, color: Color(0xFF673AB7), size: 20),
              )
            ],
          ),
          const SizedBox(height: 24),
          SizedBox(
            height: 150, // Un poco más bajo que el de BPM
            child: samples.isEmpty
                ? const Center(child: Text('Sin datos de movimiento', style: TextStyle(color: Colors.grey)))
                : LineChart(_chartData()),
          ),
        ],
      ),
    );
  }

  LineChartData _chartData() {
    // Convertimos los datos crudos a "Intensidad de movimiento"
    // Magnitud = |x| + |y| + |z| (simplificado para rendimiento)
    final spots = samples.map((s) {
      double intensity = (s.ax.abs() + s.ay.abs() + s.az.abs()).toDouble();
      // Escalamos visualmente para que quepa bien en la gráfica (ajusta el /1000 según tu sensor)
      return FlSpot(s.timestampMs / 1000.0, intensity);
    }).toList();

    if (spots.isEmpty) return LineChartData();

    final double minX = spots.first.x;
    final double maxX = spots.last.x;

    return LineChartData(
      minX: minX,
      maxX: maxX,
      minY: 0,
      // Dejamos que el maxY sea dinámico o fijo según prefieras
      gridData: FlGridData(show: false), // Limpio, sin rejilla
      borderData: FlBorderData(show: false),
      titlesData: const FlTitlesData(
        leftTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
        topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
        rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
        bottomTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)), // Sin tiempo abajo para no recargar (ya está en BPM)
      ),
      lineBarsData: [
        LineChartBarData(
          spots: spots,
          isCurved: true,
          curveSmoothness: 0.2, // Menos suavizado para mostrar picos de movimiento
          color: const Color(0xFF673AB7), // Morado Deep Purple
          barWidth: 2,
          isStrokeCapRound: true,
          dotData: const FlDotData(show: false),
          belowBarData: BarAreaData(
            show: true,
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: [
                const Color(0xFF673AB7).withOpacity(0.4),
                const Color(0xFF673AB7).withOpacity(0.0),
              ],
            ),
          ),
        ),
      ],
    );
  }
}