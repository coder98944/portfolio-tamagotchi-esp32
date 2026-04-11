package com.example.tamagotchiiot;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.lifecycle.ViewModelProvider;

import com.tamagotchi.iot.bluetooth.ConnectionState;
import com.tamagotchi.iot.viewmodel.MainViewModel;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class MainActivity extends AppCompatActivity {

    private MainViewModel viewModel;
    private TextView tvStatus;
    private TextView tvMessages;
    private Button btnConnect;
    private Button btnFeed;
    private Button btnPlay;
    private Button btnSleep;

    // Cheat UI
    private SeekBar sbHunger, sbPlay, sbSleep;
    private TextView tvHungerVal, tvPlayVal, tvSleepVal;
    private Button btnApplyCheat;

    private final ActivityResultLauncher<String[]> requestPermissionLauncher =
            registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(), this::onPermissionsResult);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        // 1. Inicializar UI Básica (Vinculación de IDs)
        tvStatus = findViewById(R.id.tv_status);
        tvMessages = findViewById(R.id.tv_messages);
        btnConnect = findViewById(R.id.btn_connect);
        
        btnFeed = findViewById(R.id.btn_feed);
        btnPlay = findViewById(R.id.btn_play);
        btnSleep = findViewById(R.id.btn_sleep);

        // 2. Inicializar UI Cheat
        sbHunger = findViewById(R.id.sb_hunger);
        sbPlay = findViewById(R.id.sb_play);
        sbSleep = findViewById(R.id.sb_sleep);
        tvHungerVal = findViewById(R.id.tv_hunger_val);
        tvPlayVal = findViewById(R.id.tv_play_val);
        tvSleepVal = findViewById(R.id.tv_sleep_val);
        btnApplyCheat = findViewById(R.id.btn_apply_cheat);

        // Configurar Listeners de SeekBars
        setupSeekBarListeners();

        // 3. Inicializar ViewModel
        viewModel = new ViewModelProvider(this).get(MainViewModel.class);

        // 4. Observadores
        setupObservers();

        // 5. Asignar Listeners a Botones (Con Logs y Toasts de Depuración)
        setupButtonListeners();

        checkBluetoothPermissions();
    }

    private void setupObservers() {
        // Observar Estado de Conexión
        viewModel.connectionState.observe(this, state -> {
            switch (state) {
                case CONNECTED:
                    tvStatus.setText("ESTADO: CONECTADO");
                    tvStatus.setTextColor(Color.GREEN);
                    btnConnect.setText("DESCONECTAR");
                    btnConnect.setBackgroundColor(Color.parseColor("#D32F2F")); // Rojo
                    btnConnect.setEnabled(true);
                    break;
                case CONNECTING:
                    tvStatus.setText("ESTADO: CONECTANDO...");
                    tvStatus.setTextColor(Color.YELLOW);
                    btnConnect.setText("CONECTANDO...");
                    btnConnect.setBackgroundColor(Color.parseColor("#FFA000")); // Naranja
                    btnConnect.setEnabled(false);
                    break;
                case DISCONNECTED:
                    tvStatus.setText("ESTADO: DESCONECTADO");
                    tvStatus.setTextColor(Color.WHITE);
                    btnConnect.setText("CONECTAR");
                    btnConnect.setBackgroundColor(Color.parseColor("#00E5FF")); // Cyan
                    btnConnect.setEnabled(true);
                    break;
                case ERROR:
                    tvStatus.setText("ESTADO: ERROR");
                    tvStatus.setTextColor(Color.RED);
                    btnConnect.setText("CONECTAR");
                    btnConnect.setBackgroundColor(Color.parseColor("#00E5FF")); // Cyan
                    btnConnect.setEnabled(true);
                    break;
            }
        });

        // Observar Mensajes Entrantes
        viewModel.incomingMessage.observe(this, message -> {
            tvMessages.setText("> " + message);
        });
    }

    private void setupButtonListeners() {
        // Botón Conectar
        btnConnect.setOnClickListener(v -> {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Botón CONECTAR presionado.");
            if (viewModel.connectionState.getValue() == ConnectionState.CONNECTED) {
                viewModel.disconnect();
            } else {
                connectToFirstPairedDevice();
            }
        });

        // Botón COMER
        btnFeed.setOnClickListener(v -> {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Botón COMER presionado.");
            // Toast.makeText(this, "Botón COMER presionado", Toast.LENGTH_SHORT).show(); // Visual feedback
            viewModel.sendFeedCommand();
        });

        // Botón JUGAR
        btnPlay.setOnClickListener(v -> {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Botón JUGAR presionado.");
            viewModel.sendPlayCommand();
        });

        // Botón DORMIR
        btnSleep.setOnClickListener(v -> {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Botón DORMIR presionado.");
            viewModel.sendSleepCommand();
        });
        
        // Botón CHEAT
        btnApplyCheat.setOnClickListener(v -> {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Botón CHEAT presionado.");
            int h = sbHunger.getProgress();
            int p = sbPlay.getProgress();
            int s = sbSleep.getProgress();
            viewModel.sendCheatCommand(h, p, s);
            Toast.makeText(this, "Cheat Enviado: " + h + "," + p + "," + s, Toast.LENGTH_SHORT).show();
        });
    }

    private void setupSeekBarListeners() {
        sbHunger.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvHungerVal.setText("Hambre: " + progress + "%");
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        sbPlay.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvPlayVal.setText("Felicidad: " + progress + "%");
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        sbSleep.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvSleepVal.setText("Energía: " + progress + "%");
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
    }

    private void connectToFirstPairedDevice() {
        android.util.Log.d("DEBUG_TAG", "MainActivity: Iniciando búsqueda de dispositivos emparejados.");
        
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) {
            android.util.Log.e("DEBUG_TAG", "MainActivity: Bluetooth no disponible en este dispositivo.");
            Toast.makeText(this, "Bluetooth no disponible", Toast.LENGTH_SHORT).show();
            return;
        }

        // CORRECCIÓN: Solo verificar BLUETOOTH_CONNECT en Android 12+ (SDK 31+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                android.util.Log.e("DEBUG_TAG", "MainActivity: No hay permisos de BLUETOOTH_CONNECT (Android 12+).");
                checkBluetoothPermissions();
                return;
            }
            android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso BLUETOOTH_CONNECT: OTORGADO (Android 12+)");
        } else {
            // Android 10 y anteriores: Solo verificamos ACCESS_FINE_LOCATION
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                android.util.Log.e("DEBUG_TAG", "MainActivity: No hay permisos de ACCESS_FINE_LOCATION (Android 10-).");
                checkBluetoothPermissions();
                return;
            }
            android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso ACCESS_FINE_LOCATION: OTORGADO (Android 10-)");
        }
        
        android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso BLUETOOTH_CONNECT: OTORGADO");
        
        // Verificar si Bluetooth está encendido
        if (!bluetoothAdapter.isEnabled()) {
            android.util.Log.e("DEBUG_TAG", "MainActivity: Bluetooth está APAGADO.");
            Toast.makeText(this, "Por favor, enciende el Bluetooth", Toast.LENGTH_LONG).show();
            return;
        }
        
        android.util.Log.d("DEBUG_TAG", "MainActivity: Bluetooth está ENCENDIDO");

        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();
        android.util.Log.d("DEBUG_TAG", "MainActivity: Se encontraron " + pairedDevices.size() + " dispositivos emparejados.");
        
        BluetoothDevice targetDevice = null;

        if (pairedDevices.size() > 0) {
            for (BluetoothDevice device : pairedDevices) {
                String deviceName = device.getName();
                android.util.Log.d("DEBUG_TAG", "MainActivity: Revisando dispositivo: " + deviceName + " [" + device.getAddress() + "]");
                
                if (deviceName != null && deviceName.toLowerCase().trim().contains("tamagotchi_iot")) {
                    android.util.Log.d("DEBUG_TAG", "MainActivity: ¡Dispositivo encontrado! " + deviceName);
                    targetDevice = device;
                    break;
                }
            }
        }

        if (targetDevice != null) {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Llamando a viewModel.connectToDevice()");
            viewModel.connectToDevice(targetDevice);
            Toast.makeText(this, "Conectando a: " + targetDevice.getName(), Toast.LENGTH_SHORT).show();
        } else {
            android.util.Log.e("DEBUG_TAG", "MainActivity: No se encontró ningún dispositivo con 'tamagotchi_iot' en el nombre.");
            Toast.makeText(this, "No se encontró 'Tamagotchi_IoT' emparejado. Por favor empareje el dispositivo primero.", Toast.LENGTH_LONG).show();
        }
    }

    private void checkBluetoothPermissions() {
        List<String> permissionsNeeded = new ArrayList<>();
        
        android.util.Log.d("DEBUG_TAG", "MainActivity: Verificando permisos de Bluetooth. SDK: " + Build.VERSION.SDK_INT);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso BLUETOOTH_CONNECT no otorgado, solicitando...");
                permissionsNeeded.add(Manifest.permission.BLUETOOTH_CONNECT);
            } else {
                android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso BLUETOOTH_CONNECT ya otorgado");
            }
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso BLUETOOTH_SCAN no otorgado, solicitando...");
                permissionsNeeded.add(Manifest.permission.BLUETOOTH_SCAN);
            } else {
                android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso BLUETOOTH_SCAN ya otorgado");
            }
        } else {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso ACCESS_FINE_LOCATION no otorgado, solicitando...");
                permissionsNeeded.add(Manifest.permission.ACCESS_FINE_LOCATION);
            } else {
                android.util.Log.d("DEBUG_TAG", "MainActivity: Permiso ACCESS_FINE_LOCATION ya otorgado");
            }
        }

        if (!permissionsNeeded.isEmpty()) {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Solicitando " + permissionsNeeded.size() + " permisos");
            requestPermissionLauncher.launch(permissionsNeeded.toArray(new String[0]));
        } else {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Todos los permisos ya están otorgados");
        }
    }

    private void onPermissionsResult(Map<String, Boolean> result) {
        android.util.Log.d("DEBUG_TAG", "MainActivity: Resultado de permisos:");
        
        boolean allGranted = true;
        for (Map.Entry<String, Boolean> entry : result.entrySet()) {
            android.util.Log.d("DEBUG_TAG", "  - " + entry.getKey() + ": " + (entry.getValue() ? "OTORGADO" : "DENEGADO"));
            if (!entry.getValue()) {
                allGranted = false;
            }
        }

        if (!allGranted) {
            android.util.Log.e("DEBUG_TAG", "MainActivity: NO se otorgaron todos los permisos");
            Toast.makeText(this, "⚠️ IMPORTANTE: Debes otorgar TODOS los permisos de Bluetooth para que la app funcione.\n\nVe a: Ajustes → Apps → TamagotchiIoT → Permisos", Toast.LENGTH_LONG).show();
        } else {
            android.util.Log.d("DEBUG_TAG", "MainActivity: Todos los permisos otorgados correctamente");
            Toast.makeText(this, "✓ Permisos otorgados. Ahora intenta conectar.", Toast.LENGTH_SHORT).show();
        }
    }
}