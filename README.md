# Mouse Radial Menu

A Linux utility that creates a radial menu overlay for mouse input devices. It allows you to assign keyboard actions to directional selections on a radial menu triggered by a mouse button.

## Dependencies

- CMake (≥3.16)
- C++17 compatible compiler
- Linux kernel headers
- uinput kernel module
- Qt6

## Building

1. Clone the repository:
```bash
git clone https://github.com/alejogrillor02/mouse-radial-menu.git
cd mouse-radial-menu
```

2. Create a build directory:
```bash
mkdir build
cd build
```

3. Run CMake and build:
```bash
cmake ..
make
```

## Usage

### Basic Usage

Run the program, either alone, with a ```config.json``` at the program's directory, or explicitly specifying a config file:

```bash
./radialmenu path/to/config.json
```

**Note:** A valid config file is required. The program will exit with error code 1 if no config file is provided or if the config file is invalid.

**Important:** You need access to /dev/input/event* and /dev/uinput.

### Configuration File (config.json)

Create a `config.json` file with the following structure:

```json
{
    "device": "/dev/input/event16",
    "trigger_button": 276,
    "selection_mode": "mouse",
    "inner_radius": 130,
    "outer_radius": 280,
    "items": [
        {
            "label": "1", "key": "KEY_1"
        },
        {
            "label": "2", "key": "KEY_2"
        },
        {
            "label": "3", "key": "KEY_3"
        },
        {
            "label": "4", "key": "KEY_4"
        },
        {
            "label": "5", "key": "KEY_5"
        },
        {
            "label": "6", "key": "KEY_6"
        },
        {
            "label": "7", "key": "KEY_7"
        },
        {
            "label": "8", "key": "KEY_8"
        }
    ]
}
```

### Configuration Options

- **device** (string): Path to the input device (e.g., `/dev/input/event16`). Leave empty for auto-detection.
- **trigger_button** (integer): The button code that triggers the menu (default: 276 = BTN_EXTRA).
- **selection_mode** (string): How to select items - `"mouse"` for mouse position or `"scroll"` for scroll wheel.
- **inner_radius** (integer): Inner radius of the radial menu in pixels (default: 130).
- **outer_radius** (integer): Outer radius of the radial menu in pixels (default: 280).
- **items** (array): Array of menu items to display and their associated keys.
  - **label** (string): Display text for the menu item.
  - **key** (string): The keyboard key to simulate when selected (e.g., `"KEY_1"`, `"KEY_A"`, `"KEY_ENTER"`).

### Finding Your Device

To list available input devices, run:
```bash
./radialmenu --list
```

This will show all pointer-like devices and highlight which ones have the trigger button you've configured.
