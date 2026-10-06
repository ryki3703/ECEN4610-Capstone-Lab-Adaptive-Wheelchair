import 'package:flutter/material.dart';
import 'package:flutter_joystick/flutter_joystick.dart';
import 'package:mjpeg_view/mjpeg_view.dart';
import 'dart:async';
import 'package:http/http.dart' as http;

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

  Timer? driveTimer;

  int x = 0;
  int y = 0;

  int speed = 0; // 0 = Slow, 1 = Medium, 2 = Max
  int sequence = 0;

  // Prevent overlapping HTTP drive requests.
  bool driveRequestInFlight = false;
  bool driveUpdatePending = false;

  // Temporary nonzero session ID for this app launch.
  final int session = DateTime.now().millisecondsSinceEpoch;

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
    int newX = (-rawX * 1000).round().clamp(-1000, 1000);
    int newY = (-rawY * 1000).round().clamp(-1000, 1000);

    // Ignore tiny movements around the center.
    const int deadband = 30;

    if (newX.abs() < deadband) {
      newX = 0;
    }

    if (newY.abs() < deadband) {
      newY = 0;
    }

    // Don't rebuild the UI if the values haven't changed.
    if (newX == x && newY == y) {
      return;
    }

    setState(() {
      x = newX;
      y = newY;
    });
  }

  void changeSpeed() {
    setState(() {
      speed = (speed + 1) % 3;
    });
  }

  Future<void> sendDriveState() async {
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
        'DRIVE $sendX,$sendY -> ${response.statusCode}',
      );
    } catch (e) {
      debugPrint('DRIVE ERROR: $e');
    } finally {
      driveRequestInFlight = false;

      // Immediately send the latest joystick state if anything
      // changed while the previous request was running.
      if (driveUpdatePending) {
        driveUpdatePending = false;
        sendDriveState();
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
                // Camera
                AspectRatio(
                  aspectRatio: 4 / 3,
                  child: ClipRRect(
                    borderRadius: BorderRadius.circular(12),
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
                          child: Text('Camera connection failed'),
                        );
                      },
                    ),
                  ),
                ),

                const SizedBox(height: 20),

                // Current joystick values
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
                      speed == 0 ? 'SPEED: SLOW' : speed == 1 ? 'SPEED: MEDIUM' : 'SPEED: MAX',
                      style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold,),
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

                    // Update joystick state quickly while dragging.
                    period: const Duration(milliseconds: 20),

                    listener: (details) {
                      updateJoystick(
                        details.x,
                        details.y,
                      );
                    },
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