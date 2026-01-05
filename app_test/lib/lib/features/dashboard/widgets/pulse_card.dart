import 'package:flutter/material.dart';
import '../../../data/sensors/pulse/pulse_state.dart';

class PulseCard extends StatelessWidget {
  final PulseState state;

  const PulseCard({
    super.key,
    required this.state,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'PULSE',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 12),
            _row('Samples', state.totalSamples.toString()),
            _row(
              'Heart Rate (BPM)',
              state.heartRate != null
                  ? state.heartRate!.toStringAsFixed(1)
                  : '—',
            ),
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
