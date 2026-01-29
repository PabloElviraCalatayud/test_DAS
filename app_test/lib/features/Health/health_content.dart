import 'dart:async';
import 'package:flutter/material.dart';

import '../../data/health/health_store.dart';
import '../../data/health/health_state.dart';
import '../../data/sensors/pulse/pulse_store.dart';
import '../../data/sensors/pulse/pulse_sample.dart';

import '../../shared/widgets/bpm_history_chart.dart';
import '../../shared/widgets/health_card.dart';
import '../../shared/widgets/sleep_score_card.dart';

class HealthContent extends StatefulWidget {
  final HealthStore healthStore;
  final PulseStore pulseStore;

  const HealthContent({
    super.key,
    required this.healthStore,
    required this.pulseStore,
  });

  @override
  State<HealthContent> createState() => _HealthContentState();
}

class _HealthContentState extends State<HealthContent> {
  StreamSubscription? _healthSub;
  final List<PulseSample> _history = [];

  @override
  void initState() {
    super.initState();

    _healthSub = widget.healthStore.stream.listen((_) {
      final avgBpm = widget.healthStore.state.avgBpm;

      if (avgBpm > 0) {
        _history.add(
          PulseSample(
            raw: 0,
            timestampMs: DateTime.now().millisecondsSinceEpoch,
            bpm: avgBpm,
          ),
        );
      }

      if (mounted) setState(() {});
    });
  }

  @override
  void dispose() {
    _healthSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final HealthState healthState = widget.healthStore.state;

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        HealthCard(
          title: 'Pulso cardíaco',
          value: healthState.avgBpm == 0
              ? '—'
              : '${healthState.avgBpm.round()} BPM',
          subtitle: _pulseLabel(healthState.avgBpm),
          icon: Icons.favorite,
          trailing: _calibrationChip(
            context,
            '±${widget.pulseStore.bpmOffset.round()} BPM',
          ),
          onTap: _calibratePulse,
        ),
        const SizedBox(height: 16),

        HealthCard(
          title: 'Eventos de apnea',
          value: '${healthState.apneaCount}',
          subtitle: _apneaLabel(healthState.apneaCount),
          icon: Icons.air,
          trailing: Column(
            crossAxisAlignment: CrossAxisAlignment.end,
            children: [
              _calibrationChip(
                context,
                'ΔBPM ${healthState.bpmDropThreshold.round()}',
              ),
              const SizedBox(height: 4),
              _calibrationChip(
                context,
                'Mov ${healthState.movementThreshold.toStringAsFixed(1)}',
              ),
            ],
          ),
          onTap: _calibrateApnea,
        ),
        const SizedBox(height: 16),

        BpmHistoryChart(
          samples: _history,
        ),
        const SizedBox(height: 16),

        SleepScoreCard(
          score: healthState.sleepScore,
          movementIndex: healthState.movementIndex,
        ),
      ],
    );
  }

  Widget _calibrationChip(BuildContext context, String text) {
    final scheme = Theme.of(context).colorScheme;

    return Chip(
      label: Text(
        text,
        style: Theme.of(context).textTheme.bodySmall,
      ),
      backgroundColor: scheme.primary.withOpacity(0.12),
      labelStyle: TextStyle(
        color: scheme.primary,
        fontWeight: FontWeight.w600,
      ),
      visualDensity: VisualDensity.compact,
    );
  }
//Tienen que permitir ajustar el offset para el calculo de las pulsaciones porque da lecturas de 170 pulsaciones y hay que incluir un parametro de calibracion que sume/reste pulsaciones
// El resto del codigo relevante esta en la carpeta de sensores y la carpeta de health.
  void _calibratePulse() {
    showDialog(
      context: context,
      builder: (_) {
        double offset = widget.pulseStore.bpmOffset;

        return StatefulBuilder(
          builder: (context, setLocalState) {
            return AlertDialog(
              title: const Text('Calibrar pulso'),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    'Offset: ${offset.round()} BPM',
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                  Slider(
                    min: -100,
                    max: 100,
                    divisions: 200,
                    value: offset,
                    label: offset.round().toString(),
                    onChanged: (v) {
                      setLocalState(() {
                        offset = v;
                      });
                      widget.pulseStore.setBpmOffset(v);
                    },
                  ),
                ],
              ),
            );
          },
        );
      },
    );
  }

  void _calibrateApnea() {
    showDialog(
      context: context,
      builder: (_) {
        double bpmDrop = widget.healthStore.state.bpmDropThreshold;
        double movement = widget.healthStore.state.movementThreshold;

        return StatefulBuilder(
          builder: (context, setLocalState) {
            return AlertDialog(
              title: const Text('Calibrar apnea'),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    'Caída de BPM: ${bpmDrop.round()}',
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                  Slider(
                    min: 1,
                    max: 15,
                    divisions: 14,
                    value: bpmDrop,
                    label: bpmDrop.round().toString(),
                    onChanged: (v) {
                      setLocalState(() {
                        bpmDrop = v;
                      });
                      widget.healthStore.setApneaThresholds(
                        bpmDrop: bpmDrop,
                        movement: movement,
                      );
                    },
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Movimiento: ${movement.toStringAsFixed(1)}',
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                  Slider(
                    min: 0.5,
                    max: 10,
                    divisions: 19,
                    value: movement,
                    label: movement.toStringAsFixed(1),
                    onChanged: (v) {
                      setLocalState(() {
                        movement = v;
                      });
                      widget.healthStore.setApneaThresholds(
                        bpmDrop: bpmDrop,
                        movement: movement,
                      );
                    },
                  ),
                ],
              ),
            );
          },
        );
      },
    );
  }

  String _pulseLabel(double bpm) {
    if (bpm == 0) return 'Sin datos';
    if (bpm < 50) return 'Bajo';
    if (bpm <= 100) return 'Normal';
    return 'Alto';
  }

  String _apneaLabel(int count) {
    if (count == 0) return 'Sin eventos';
    if (count < 3) return 'Leve';
    if (count < 6) return 'Moderado';
    return 'Severo';
  }
}
