import '../../data/thingsboard/thingsboard_service.dart';
import '../../data/thingsboard/thingsboard_service_config.dart';

final tbService = ThingsBoardService(
  server: ThingsBoardConfig.server,
  accessToken: ThingsBoardConfig.mobileAccessToken,
);