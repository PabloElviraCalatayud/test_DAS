import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

import '../../data/sensors/pulse/pulse_sample.dart';

class BpmHistoryChart extends StatelessWidget {
  final List<PulseSample> samples;

  const BpmHistoryChart({
    super.key,
    required this.samples,
  });

  static const double _minBpmChart = 40;
  static const double _maxBpmChart = 190;

  @override
  Widget build(BuildContext context) {
    return Card(
      elevation: 2,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
      ),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Historial de pulsaciones',
              style: TextStyle(
                fontSize: 16,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: 220,
              child: samples.isEmpty
                  ? const Center(
                child: Text(
                  'Sin datos de pulsaciones',
                  style: TextStyle(color: Colors.grey),
                ),
              )
                  : LineChart(_chartData()),
            ),
          ],
        ),
      ),
    );
  }

  LineChartData _chartData() {
    if (samples.isEmpty) {
      return LineChartData();
    }

    final spots = <FlSpot>[];

    for (final s in samples) {
      spots.add(
        FlSpot(
          s.timestampMs / 1000.0,
          s.bpm.clamp(_minBpmChart, _maxBpmChart),
        ),
      );
    }

    final double minX = spots.first.x;
    final double maxX = spots.last.x;

    final double timeSpan = maxX - minX;
    final double xInterval = _computeTimeInterval(timeSpan);

    return LineChartData(
      minX: minX,
      maxX: maxX,
      minY: _minBpmChart,
      maxY: _maxBpmChart,
      gridData: FlGridData(
        show: true,
        horizontalInterval: 20,
        verticalInterval: xInterval,
        getDrawingHorizontalLine: (value) {
          return FlLine(
            color: Colors.grey.withOpacity(0.15),
            strokeWidth: 1,
          );
        },
        getDrawingVerticalLine: (value) {
          return FlLine(
            color: Colors.grey.withOpacity(0.15),
            strokeWidth: 1,
          );
        },
      ),
      borderData: FlBorderData(
        show: true,
        border: Border.all(
          color: Colors.grey.withOpacity(0.2),
          width: 1,
        ),
      ),
      lineTouchData: LineTouchData(
        enabled: true,
        touchTooltipData: LineTouchTooltipData(
          getTooltipItems: (touchedSpots) {
            return touchedSpots.map((spot) {
              final time = DateTime.fromMillisecondsSinceEpoch(
                (spot.x * 1000).toInt(),
              );
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
        topTitles: const AxisTitles(
          sideTitles: SideTitles(showTitles: false),
        ),
        rightTitles: const AxisTitles(
          sideTitles: SideTitles(showTitles: false),
        ),
        bottomTitles: AxisTitles(
          sideTitles: SideTitles(
            showTitles: true,
            reservedSize: 32,
            interval: xInterval,
            getTitlesWidget: (value, meta) {
              final time = DateTime.fromMillisecondsSinceEpoch(
                (value * 1000).toInt(),
              );
              return Padding(
                padding: const EdgeInsets.only(top: 6),
                child: Text(
                  DateFormat('HH:mm').format(time),
                  style: const TextStyle(
                    color: Colors.grey,
                    fontSize: 10,
                  ),
                ),
              );
            },
          ),
        ),
        leftTitles: AxisTitles(
          axisNameWidget: const Padding(
            padding: EdgeInsets.only(bottom: 8),
            child: Text(
              'BPM',
              style: TextStyle(
                fontSize: 11,
                color: Colors.grey,
              ),
            ),
          ),
          axisNameSize: 24,
          sideTitles: SideTitles(
            showTitles: true,
            reservedSize: 42,
            interval: 20,
            getTitlesWidget: (value, meta) {
              return Text(
                value.toInt().toString(),
                style: const TextStyle(
                  color: Colors.grey,
                  fontSize: 10,
                ),
                textAlign: TextAlign.right,
              );
            },
          ),
        ),
      ),
      lineBarsData: [
        LineChartBarData(
          spots: spots,
          isCurved: true,
          curveSmoothness: 0.35,
          color: Colors.red,
          barWidth: 2.5,
          dotData: const FlDotData(show: false),
          belowBarData: BarAreaData(
            show: true,
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: [
                Colors.red.withOpacity(0.2),
                Colors.red.withOpacity(0.05),
              ],
            ),
          ),
        ),
      ],
    );
  }

  double _computeTimeInterval(double spanSeconds) {
    if (spanSeconds <= 60) {
      return 10;
    }
    if (spanSeconds <= 5 * 60) {
      return 30;
    }
    if (spanSeconds <= 15 * 60) {
      return 60;
    }
    if (spanSeconds <= 60 * 60) {
      return 5 * 60;
    }
    return 15 * 60;
  }
}
