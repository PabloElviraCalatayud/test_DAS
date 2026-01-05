import 'package:flutter/material.dart';
import '../../../data/sensors/imu/imu_state.dart';

class ImuCard extends StatelessWidget {
  final ImuState state;

  const ImuCard({
    super.key,
    required this.state,
  });

  @override
  Widget build(BuildContext context) {
    final last = state.lastSample;

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'IMU',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 12),
            _row('Samples', state.totalSamples.toString()),
            _row('ax', last?.ax.toString() ?? '—'),
            _row('ay', last?.ay.toString() ?? '—'),
            _row('az', last?.az.toString() ?? '—'),
            _row('gx', last?.gx.toString() ?? '—'),
            _row('gy', last?.gy.toString() ?? '—'),
            _row('gz', last?.gz.toString() ?? '—'),
          ],
        ),
      ),
    );
  }

  Widget _row(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label),
          Text(value),
        ],
      ),
    );
  }
}
