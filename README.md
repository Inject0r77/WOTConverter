<p align="center">
  <img src="resources/app_icon.png" width="96" alt="WoT Converter icon">
</p>

<h1 align="center">WoT Converter</h1>

<p align="center">
  Native Windows converter for World of Tanks resources.<br>
  LESTA & WG • RU / EN • C++20 / Win32
</p>

<p align="center">
  <img alt="Version" src="https://img.shields.io/badge/version-1.1.2-9B6CFF?style=flat-square">
  <img alt="Platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11-1F6FEB?style=flat-square">
  <img alt="C++" src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square">
</p>

<p align="center">
  <a href="#-русский">Русский</a> · <a href="#-english">English</a>
</p>

---

## 🇷🇺 Русский

**WoT Converter** — компактный нативный конвертер ресурсов World of Tanks для Windows. Значения пересчитываются синхронно: меняете одно поле — остальные обновляются автоматически.

### Возможности

- Конвертация **RUB/EUR ↔ Gold ↔ Credits ↔ Free XP**.
- Поддержка регионов **LESTA** и **WG**.
- Модификатор свободного опыта **1:35**.
- Скидка премиум-магазина **−15%**.
- Расчёт стоимости контейнеров; для WG — подбор **минимальной стоимости пакетов**.
- Редактируемые курсы в `config/rates.json` с безопасными встроенными значениями по умолчанию.
- Интерфейс **RU / EN** и сохранение пользовательских настроек.
- Нативный интерфейс Win32 в визуальном стиле **CoD RCE Guard** — без Python, Electron, Qt и .NET Runtime.

### Запуск

Готовую сборку можно взять во вкладке **Releases**. Для работы дополнительные runtime-зависимости не требуются.

### Сборка из исходников

Нужно:

- Windows 10/11 x64;
- Visual Studio 2022;
- workload **Desktop development with C++**;
- CMake tools for Windows.

Самый простой вариант:

```bat
build-release.bat
```

Или вручную:

```bat
cmake -S . -B build -A x64 -DWOT_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Готовый файл:

```text
build\Release\WoTConverter.exe
```

### Настройка курсов

Курсы находятся в:

```text
config/rates.json
```

Если файл отсутствует или повреждён, приложение использует встроенные безопасные значения.

### Changelog — v1.1.2

- MSVC Runtime теперь статически линкуется (`/MT` в Release, `/MTd` в Debug).
- Готовый Release EXE не требует отдельно установленный Microsoft Visual C++ Redistributable.

### v1.1.1

- Интерфейс переработан в стиле CoD RCE Guard.
- Увеличены шрифты и иконки.
- Настройки перенесены в кнопку ⚙.
- Переключатель региона LESTA/WG оставлен в верхней панели.
- RU/EN перенесён в настройки.
- Обновлена страница «О программе».
- Добавлена фирменная иконка с двумя зацикленными стрелками.

---

## 🇬🇧 English

**WoT Converter** is a compact native Windows utility for converting World of Tanks resources. Edit any field and the remaining values are recalculated automatically.

### Features

- **RUB/EUR ↔ Gold ↔ Credits ↔ Free XP** conversion.
- **LESTA** and **WG** region support.
- **1:35** Free XP modifier.
- **−15%** Premium Shop discount modifier.
- Box cost calculator; WG mode finds the **minimum-cost package combination**.
- Editable rates in `config/rates.json` with safe built-in fallback values.
- **RU / EN** interface with persistent user settings.
- Native Win32 UI inspired by **CoD RCE Guard** — no Python, Electron, Qt or .NET Runtime required.

### Run

Prebuilt binaries are available on the **Releases** page. No additional runtime dependencies are required.

### Build from source

Requirements:

- Windows 10/11 x64;
- Visual Studio 2022;
- **Desktop development with C++** workload;
- CMake tools for Windows.

Easiest way:

```bat
build-release.bat
```

Or manually:

```bat
cmake -S . -B build -A x64 -DWOT_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Output:

```text
build\Release\WoTConverter.exe
```

### Rates configuration

Rates are stored in:

```text
config/rates.json
```

If the file is missing or invalid, the application falls back to built-in safe defaults.

### Changelog — v1.1.2

- MSVC Runtime is now linked statically (`/MT` for Release, `/MTd` for Debug).
- The Release EXE no longer requires a separately installed Microsoft Visual C++ Redistributable.

### v1.1.1

- Reworked the UI in the CoD RCE Guard visual style.
- Increased typography and icon sizes.
- Moved Settings to the ⚙ button.
- Kept the LESTA/WG region selector in the top bar.
- Moved RU/EN selection into Settings.
- Reworked the About page.
- Added a dedicated loop-arrows application icon.

---

<p align="center">
  <sub>Unofficial community utility. Not affiliated with or endorsed by Lesta Games or Wargaming.</sub>
</p>
