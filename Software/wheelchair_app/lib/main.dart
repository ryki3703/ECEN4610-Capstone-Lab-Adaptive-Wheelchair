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

  // Temporary nonzero session ID for this app launch.
  // We'll replace this with a proper generated value.
  final int session = DateTime.now().millisecondsSinceEpoch;
  @override
  void initState() {
    super.initState();

    driveTimer = Timer.periodic(
      const Duration(milliseconds: 50),
      (_) => sendDriveState(),
    );
  }

  void updateJoystick(double rawX, double rawY) {
    setState(() {
      x = (rawX * 1000).round().clamp(-1000, 1000);
      y = (rawY * 1000).round().clamp(-1000, 1000);
    });
  }

  void resetJoystick() {
    setState(() {
      x = 0;
      y = 0;
    });
  }

  Future<void> sendDriveState() async {
    sequence++;

    final uri = Uri.parse(
      '$driveUrl'
      '?x=$x'
      '&y=$y'
      '&speed=$speed',
    );

    debugPrint('SENDING: $uri');

    try {
      final response = await http
          .get(uri)
          .timeout(const Duration(milliseconds: 250));

      debugPrint(
        'RESPONSE: ${response.statusCode} ${response.body}',
      );
    } catch (e) {
      debugPrint('DRIVE ERROR: $e');
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

                // Joystick
                SizedBox(
                  width: 250,
                  height: 250,
                  child: Joystick(
                    mode: JoystickMode.all,
                    listener: (details) {
                      updateJoystick(details.x, details.y);
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