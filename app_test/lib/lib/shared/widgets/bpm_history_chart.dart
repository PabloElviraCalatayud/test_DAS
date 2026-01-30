import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import '../../data/sensors/pulse/pulse_sample.dart';

class BpmHistoryChart extends StatelessWidget {
  final List<PulseSample> samples;

  const BpmHistoryChart({super.key, required this.samples});

  static const double _minBpmChart = 40;
  static const double _maxBpmChart = 190;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    // Si bpm es nullable, filtramos.
    final valid = samples.where((s) {
      final bpm = s.bpm;
      return bpm != null && bpm > 0;
    }).toList();

    final lastBpmText = valid.isNotEmpty ? '${valid.last.bpm!.round()} BPM' : '--';

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
              Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Frecuencia Cardíaca',
                    style: TextStyle(
                      fontSize: 14,
                      color: Colors.grey,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    lastBpmText,
                    style: const TextStyle(
                      fontSize: 24,
                      fontWeight: FontWeight.bold,
                      color: Color(0xFF2B5C9E),
                    ),
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
            child: valid.isEmpty
                ? const Center(
              child: Text(
                'Esperando datos...',
                style: TextStyle(color: Colors.grey),
              ),
            )
                : LineChart(_chartData(theme, valid)),
          ),
        ],
      ),
    );
  }

  LineChartData _chartData(ThemeData theme, List<PulseSample> valid) {
    final spots = <FlSpot>[];

    for (final s in valid) {
      final bpm = s.bpm!;
      spots.add(
        FlSpot(
          s.timestampMs / 1000.0,
          bpm.clamp(_minBpmChart, _maxBpmChart).toDouble(),
        ),
      );
    }

    if (spots.isEmpty) return LineChartData();

    double minX = spots.first.x;
    double maxX = spots.last.x;

    // Evita span 0 cuando solo hay 1 punto
    if (maxX <= minX) {
      maxX = minX + 1;
    }

    final span = maxX - minX;
    final xInterval = _computeTimeInterval(span);

    return LineChartData(
      minX: minX,
      maxX: maxX,
      minY: _minBpmChart,
      maxY: _maxBpmChart,
      gridData: FlGridData(
        show: true,
        drawVerticalLine: false,
        horizontalInterval: 40,
        getDrawingHorizontalLine: (value) => FlLine(
          color: Colors.grey.withOpacity(0.1),
          strokeWidth: 1,
          dashArray: [5, 5], // si tu versión de fl_chart no lo soporta, quítalo
        ),
      ),
      borderData: FlBorderData(show: false),
      lineTouchData: LineTouchData(
        enabled: true,
        touchTooltipData: LineTouchTooltipData(
          getTooltipItems: (touchedSpots) {
            return touchedSpots.map((spot) {
              final time = DateTime.fromMillisecondsSinceEpoch((spot.x * 1000).toInt());
              return LineTooltipItem(
                '${spot.y.toInt()} BPM\n${DateFormat('HH:mm:ss').format(time)}',
                const TextStyle(
                  color: Colors.white,
                  fontWeight: FontWeight.bold,
                  fontSize: 11,
                ),
              );
            }).toList();
          },
        ),
      ),
      titlesData: FlTitlesData(
        topTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
        rightTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
        leftTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
        bottomTitles: AxisTitles(
          sideTitles: SideTitles(
            showTitles: true,
            interval: xInterval,
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

  double _computeTimeInterval(double spanSeconds) {
    if (spanSeconds <= 60) return 10;
    if (spanSeconds <= 5 * 60) return 30;
    if (spanSeconds <= 15 * 60) return 60;
    if (spanSeconds <= 60 * 60) return 5 * 60;
    return 15 * 60;
  }
}
