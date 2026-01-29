class ImuSample {
  final int ax;
  final int ay;
  final int az;
  final int gx;
  final int gy;
  final int gz;
  final int timestampMs;

  ImuSample({
    required this.ax,
    required this.ay,
    required this.az,
    required this.gx,
    required this.gy,
    required this.gz,
    required this.timestampMs,
  });

  double get axMs2 => ax / 16384.0 * 9.81;
  double get ayMs2 => ay / 16384.0 * 9.81;
  double get azMs2 => az / 16384.0 * 9.81;

  double get gxDps => gx / 131.0;
  double get gyDps => gy / 131.0;
  double get gzDps => gz / 131.0;
}
