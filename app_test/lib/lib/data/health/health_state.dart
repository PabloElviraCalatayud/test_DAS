import 'apnea_event.dart';

class HealthState {
  double avgBpm;
  double movementIndex;
  int sleepScore;

  int apneaCount;
  double apneaProbability;
  List<ApneaEvent> apneas;

  HealthState({
    this.avgBpm = 0,
    this.movementIndex = 0,
    this.sleepScore = 0,
    this.apneaCount = 0,
    this.apneaProbability = 0,
    List<ApneaEvent>? apneas,
  }) : apneas = apneas ?? [];
}
