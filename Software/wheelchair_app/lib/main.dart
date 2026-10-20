import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_joystick/flutter_joystick.dart';
import 'package:http/http.dart' as http;
import 'package:mjpeg_view/mjpeg_view.dart';

void main() {
  runApp(const WheelchairApp());
}

class WheelchairApp extends StatelessWidget {
  const WheelchairApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Wheelchair Remote',
      theme: ThemeData(
        brightness: Brightness.dark,
        useMaterial3: true,
      ),
      home: const WheelchairScreen(),
    );
  }
}

class WheelchairScreen extends StatefulWidget {
  const WheelchairScreen({super.key});

  @override
  State<WheelchairScreen> createState() => _WheelchairScreenState();
}

class _WheelchairScreenState extends State<WheelchairScreen> {
  static const String cameraUrl = 'http://192.168.4.1:81/stream';
  static const String driveUrl = 'http://192.168.4.1/drive';

  static const MethodChannel wifiChannel =
      MethodChannel('wheelchair.wifi');

  Timer? driveTimer;

  bool wifiConnecting = false;
  bool driveRequestInFlight = false;
  bool driveUpdatePending = false;

  int x = 0;
  int y = 0;

  // 0 = Slow, 1 = Medium, 2 = Max
  int speed = 0;

  @override
  void initState() {
    super.initState();

    // Send the latest joystick state 20 times per second.
    driveTimer = Timer.periodic(
      const Duration(milliseconds: 50),
      (_) => sendDriveState(),
    );
  }

  void updateJoystick(double rawX, double rawY) {
    // Invert both axes to match the wheelchair's directions.
    int newX = (-rawX * 1000).round().clamp(-1000, 1000);
    int newY = (-rawY * 1000).round().clamp(-1000, 1000);

    // Ignore small movements around the center.
    const int deadband = 30;

    if (newX.abs() < deadband) {
      newX = 0;
    }

    if (newY.abs() < deadband) {
      newY = 0;
    }

    if (newX == x && newY == y) {
      return;
    }

    setState(() {
      x = newX;
      y = newY;
    });
  }

  void resetJoystick() {
    if (!mounted) return;

    setState(() {
      x = 0;
      y = 0;
    });
  }

  void changeSpeed() {
    setState(() {
      speed = (speed + 1) % 3;
    });
  }

  Future<void> sendDriveState() async {
    // Prevent overlapping HTTP requests.
    if (driveRequestInFlight) {
      driveUpdatePending = true;
      return;
    }

    driveRequestInFlight = true;

    final int sendX = x;
    final int sendY = y;
    final int sendSpeed = speed;

    final uri = Uri.parse(
      '$driveUrl'
      '?x=$sendX'
      '&y=$sendY'
      '&speed=$sendSpeed',
    );

    try {
      final response = await http
          .get(uri)
          .timeout(const Duration(milliseconds: 450));

      debugPrint(
        'DRIVE $sendX,$sendY SPEED $sendSpeed '
        '-> ${response.statusCode}',
      );
    } catch (e) {
      debugPrint('DRIVE ERROR: $e');
    } finally {
      driveRequestInFlight = false;

      // Send the latest state if a request was skipped.
      if (driveUpdatePending) {
        driveUpdatePending = false;
        sendDriveState();
      }
    }
  }

  Future<void> connectToWheelchair() async {
    if (wifiConnecting) return;

    setState(() {
      wifiConnecting = true;
    });

    try {
      final bool connected =
          await wifiChannel.invokeMethod<bool>(
                'connectToWheelchair',
              ) ??
              false;

      if (!mounted) return;

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            connected
                ? 'Wi-Fi connection requested successfully'
                : 'Could not connect to Wheelchair-001',
          ),
        ),
      );
    } on MissingPluginException {
      if (!mounted) return;

      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text(
            'Automatic Wi-Fi connection is not available '
            'on this platform yet.',
          ),
        ),
      );
    } on PlatformException catch (e) {
      if (!mounted) return;

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            'Wi-Fi connection failed: ${e.message}',
          ),
        ),
      );
    } finally {
      if (mounted) {
        setState(() {
          wifiConnecting = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Wheelchair Remote'),
        centerTitle: true,
      ),
      body: LayoutBuilder(
        builder: (context, constraints) {
          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                // Connect to Wheelchair button
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton(
                    onPressed:
                        wifiConnecting ? null : connectToWheelchair,
                    child: Text(
                      wifiConnecting
                          ? 'CONNECTING...'
                          : 'CONNECT TO WHEELCHAIR',
                    ),
                  ),
                ),

                const SizedBox(height: 16),

                // Camera - rotated 90 degrees clockwise
                AspectRatio(
                  aspectRatio: 3 / 4,
                  child: ClipRRect(
                    borderRadius: BorderRadius.circular(12),
                    child: Stack(
                      alignment: Alignment.center,
                      children: [
                        Positioned.fill(
                          child: RotatedBox(
                            quarterTurns: 1,
                            child: MjpegView(
                              uri: cameraUrl,
                              fit: BoxFit.contain,
                              fps: 20,

                              loadingWidget: (context) {
                                return const Center(
                                  child: CircularProgressIndicator(),
                                );
                              },

                              errorWidget: (context) {
                                return const Center(
                                  child: Text(
                                    'Camera connection failed',
                                  ),
                                );
                              },
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),

                const SizedBox(height: 20),

                // Current joystick coordinates
                Text(
                  'X: $x    Y: $y',
                  style: const TextStyle(
                    fontSize: 22,
                    fontWeight: FontWeight.bold,
                  ),
                ),

                const SizedBox(height: 20),

                // Speed control
                SizedBox(
                  width: 200,
                  child: ElevatedButton(
                    onPressed: changeSpeed,
                    child: Text(
                      speed == 0
                          ? 'SPEED: SLOW'
                          : speed == 1
                              ? 'SPEED: MEDIUM'
                              : 'SPEED: MAX',
                      style: const TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ),
                ),

                const SizedBox(height: 20),

                // Joystick
                SizedBox(
                  width: 250,
                  height: 250,
                  child: Joystick(
                    mode: JoystickMode.all,
                    period: const Duration(milliseconds: 20),
                    listener: (details) {
                      updateJoystick(
                        details.x,
                        details.y,
                      );
                    },
                    onStickDragEnd: resetJoystick,
                  ),
                ),
              ],
            ),
          );
        },
      ),
    );
  }

  @override
  void dispose() {
    driveTimer?.cancel();
    super.dispose();
  }
}