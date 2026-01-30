import 'dart:convert';
import 'package:http/http.dart' as http;

class ThingsBoardService {
  final String server;
  final String accessToken;

  ThingsBoardService({
    required this.server,
    required this.accessToken,
  });

  Future<bool> sendTelemetry(Map<String, dynamic> data) async {
    final url = Uri.parse('$server/api/v1/$accessToken/telemetry');

    try {
      final response = await http.post(
        url,
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode(data),
      );

      return response.statusCode == 200;
    } catch (e) {
      print('ThingsBoard error: $e');
      return false;
    }
  }
}