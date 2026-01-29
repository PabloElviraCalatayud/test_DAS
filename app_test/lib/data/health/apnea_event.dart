class ApneaEvent {
  final DateTime start;
  final DateTime end;
  final double minBpm;
  final double maxBpm;

  ApneaEvent({
    required this.start,
    required this.end,
    required this.minBpm,
    required this.maxBpm,
  });

  Duration get duration => end.difference(start);
}