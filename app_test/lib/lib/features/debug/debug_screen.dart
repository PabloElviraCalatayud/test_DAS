import 'package:flutter/material.dart';
import '../packet/logic/packet_decoder.dart';

class DebugScreen extends StatelessWidget {
  const DebugScreen({super.key});

  String _hexDump(List<int> data) {
    return data
        .map((b) => b.toRadixString(16).padLeft(2, '0'))
        .join(' ');
  }

  Widget _card(
      BuildContext context,
      String title,
      double height,
      Widget child,
      ) {
    final scheme = Theme.of(context).colorScheme;

    return Container(
      height: height,
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
          Expanded(child: child),
        ],
      ),
    );
  }

  Widget _row(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(fontWeight: FontWeight.w500)),
          Text(value, style: const TextStyle(fontWeight: FontWeight.w600)),
        ],
      ),
    );
  }

  String _v(num? v) => v == null ? '-' : v.toString();
  String _vf(num? v) => v == null ? '-' : v.toStringAsFixed(2);

  @override
  Widget build(BuildContext context) {
    final decoder = PacketDecoder.instance;

    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Debug BLE',
          style: TextStyle(fontWeight: FontWeight.w700),
        ),
      ),
      body: StreamBuilder(
        stream: decoder.stream,
        builder: (context, snapshot) {
          if (!snapshot.hasData) {
            return const Center(child: Text('Esperando paquetes'));
          }

          final pkt = snapshot.data!;
          final imu = pkt.imuSamples.isNotEmpty ? pkt.imuSamples.first : null;

          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                // --------------------------------------------------
                // RAW PACKAGE
                // --------------------------------------------------
                _card(
                  context,
                  'Raw package',
                  220,
                  Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      _row('Payload size', '${pkt.raw.length} bytes'),
                      const SizedBox(height: 8),
                      Expanded(
                        child: SelectableText(
                          _hexDump(pkt.raw),
                          style: const TextStyle(
                            fontFamily: 'monospace',
                            fontSize: 13,
                          ),
                        ),
                      ),
                    ],
                  ),
                ),

                // --------------------------------------------------
                // IMU (estructura fija, valores con '-')
                // --------------------------------------------------
                _card(
                  context,
                  'IMU',
                  450,
                  Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      _row(
                        'Timestamp',
                        imu == null ? '-' : '${imu.timestampMs} ms',
                      ),
                      const SizedBox(height: 6),
                      const Text('RAW'),
                      _row('ax', _v(imu?.ax)),
                      _row('ay', _v(imu?.ay)),
                      _row('az', _v(imu?.az)),
                      _row('gx', _v(imu?.gx)),
                      _row('gy', _v(imu?.gy)),
                      _row('gz', _v(imu?.gz)),
                      const SizedBox(height: 6),
                      const Text('Converted'),
                      _row('ax (m/s²)', _vf(imu?.axMs2)),
                      _row('ay (m/s²)', _vf(imu?.ayMs2)),
                      _row('az (m/s²)', _vf(imu?.azMs2)),
                      _row('gx (dps)', _vf(imu?.gxDps)),
                      _row('gy (dps)', _vf(imu?.gyDps)),
                      _row('gz (dps)', _vf(imu?.gzDps)),
                    ],
                  ),
                ),

                // --------------------------------------------------
                // PULSE
                // --------------------------------------------------
                _card(
                  context,
                  'Pulse',
                  140,
                  Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      _row(
                        'RAW',
                        pkt.pulseSamples.isEmpty
                            ? '-'
                            : pkt.pulseSamples.last.raw.toString(),
                      ),
                      _row(
                        'BPM',
                        pkt.pulseSamples.isEmpty
                            ? '-'
                            : pkt.pulseSamples.last.bpm
                            ?.toStringAsFixed(1) ??
                            '-',
                      ),
                    ],
                  ),
                ),
              ],
            ),
          );
        },
      ),
    );
  }
}
