import 'dart:async';
import 'dart:convert';
import 'package:http/http.dart' as http;
import '../../data/sensors/imu/imu_sample.dart';
import '../../data/sensors/imu/imu_store.dart';
import '../../data/sensors/pulse/pulse_sample.dart';
import '../../data/sensors/pulse/pulse_store.dart';
import '../../data/thingsboard/thingsboard_service.dart';
import '../../data/thingsboard/thingsboard_service_config.dart';
import 'service_locator.dart';
import 'dart:math';

///funciones dummies, explicaciones de como deberia ser la funcion con sensores
///comentadas.(sustituir las lineas de codigo por las comentadas y ver si se
///reciben bien los datos de los sensores)
///sendImuTelemetry es un poco mas complicada porque crea samples y los guarda
/// para enviarlos. al final del codigo esta completa

class TelemetryStarter {
  final ThingsBoardService tbService;
  final random = Random();

  TelemetryStarter()
      : tbService = ThingsBoardService(
    server: ThingsBoardConfig.server,
    accessToken: ThingsBoardConfig.mobileAccessToken,
  );

  //Se envian los datos al pulsar boton: sendPulseNow y sendImuNow
  Future<int?> sendPulseTelemetry() async  {
    //PulseStore.instance.stream.listen((pulseState){
    //cuando se tenga sensores
    final pulseState = PulseStore.instance.state;
    final pulse = pulseState.heartRate;
    if (pulse == null) return null;

    try {
      await tbService.sendTelemetry({"pulso": pulse});
      print("Pulso enviado: $pulse");
    } catch (e) {
      print("Error al enviar pulso: $e");
    }
    return pulse;
    //}); cierre de PulseStore.instance...
  }

  Future<ImuSample?> sendImuTelemetry() async {
    //Simulacion de un sample
    final sample = ImuStore.instance.state.lastSample;
    if (sample == null) return null;

    final telemetry = {
      "acc_x": sample.axMs2,
      "acc_y": sample.ayMs2,
      "acc_z": sample.azMs2,
      "gyro_x": sample.gxDps,
      "gyro_y": sample.gyDps,
      "gyro_z": sample.gzDps,
    };
    try {
      await tbService.sendTelemetry(telemetry);
      print("IMU enviada: $telemetry");
    } catch (e) {
      print("Error al enviar IMU: $e");
    }
    return sample;
  }

//Realmente deberia quedar asi:
/*
  void startImuTelemetry() {

    ImuStore.instance.stream.listen((imuState) {
      final sample = imuState.lastSample;
      if (sample == null) return;

      tbService.sendTelemetry({
        "acc_x": sample.axMs2,
        "acc_y": sample.ayMs2,
        "acc_z": sample.azMs2,
        "gyro_x": sample.gxDps,
        "gyro_y": sample.gyDps,
        "gyro_z": sample.gzDps,
      });
    });
      try {
    await tbService.sendTelemetry(telemetry);
    print("IMU enviada: $telemetry");
  } catch (e) {
    print("Error al enviar IMU: $e");
  }
  return sample;
  }
  */
}
