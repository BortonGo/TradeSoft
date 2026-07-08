# TradeSoft — Trading Terminal (C++ / Qt)

**TradeSoft** — разрабатываемый торговый терминал на C++ и Qt для анализа рынка, визуализации свечей, проверки стратегий и постепенного перехода к более чистой low-latency архитектуре.

> Статус: **Work in Progress / MVP in Development**

Проект создаётся как pet-project и инженерное портфолио с акцентом на архитектуру, real-time обработку данных, UI/UX, торговую логику, backtesting и аккуратную подготовку hot path к измерениям latency.

---

## Цели проекта

- Создать собственный trading-терминал
- Подключаться к криптобирже через REST / WebSocket
- Загружать историю и обновлять текущую свечу в realtime
- Отображать свечной график и overlay-индикаторы
- Запускать стратегии в demo / paper контуре
- Управлять риском, SL / TP и журналом сделок
- Проверять стратегии на истории через backtest
- Постепенно отделять hot path от UI, логирования, диска и лишних аллокаций

---

## Используемые технологии

- C++20
- Qt Widgets / Network / WebSockets
- CMake / CTest
- REST / WebSocket API
- JSON-конфигурация
- QLoggingCategory
- Unit-тесты без внешнего test framework
- Подготовительный low-latency слой: core events, timestamps, latency stats

---

## Текущая архитектура

На текущем этапе TradeSoft — это Qt MVP с несколькими независимыми контурами:

- `Market Data` — история, cache fallback, realtime polling / WebSocket
- `Chart / Indicators` — свечной график, EMA, RSI, ATR, Donchian
- `Trading Runtime` — strategy runner, risk manager, demo execution, trade journal
- `Backtest` — загрузка истории, прогон стратегии, статистика, графики, экспорт отчётов
- `Core Events / Latency` — подготовительный слой для будущего event pipeline и latency metrics

Актуальные диаграммы проекта ведутся в PlantUML:

- `TradeSoft UML/TradeSoft_Architecture.puml`
- `TradeSoft UML/TradeSoft_MarketDataFlow.puml`
- `TradeSoft UML/TradeSoft_TradingFlow.puml`
- `TradeSoft UML/TradeSoft_BacktestAndLatency.puml`

Главное правило рефакторинга:

```text
В hot path не должно быть disk I/O, тяжелого логирования, обновления UI-моделей,
полного пересчета индикаторов и лишних аллокаций.
```

---

## Структура проекта

```text
src/
  backtest/              backtest engine, controller, graph widget, report exporter
  controllers/           UI-facing controllers for strategy runtime
  core/                  base domain primitives, config, logging, events, latency
    events/              MarketEvent, SignalEvent, OrderEvent, FillEvent, TradeEvent
    latency/             LatencyStats, LatencyCollector
  domain/                account, order, risk, strategy, trade records
  exchange/              IExchangeClient, BingXSwapClient
  indicators/            EMA, RSI, ATR, Donchian, indicator engine
  service/               market data, execution, account, strategy, trade journal
  ui/                    Qt widgets, dialogs, table models, main window
tests/                   lightweight unit tests
resources/               app icon and Qt resources
TradeSoft UML/           UML / architecture diagrams
```

---

## Ключевые модули

### Market Data

- `MarketDataService`
- `IExchangeClient`
- `BingXSwapClient`
- `CandleSeries`
- `CandleCache`

Поддерживается:

- загрузка исторических свечей;
- cache fallback при сетевой ошибке;
- realtime polling;
- WebSocket realtime при доступности транспорта;
- нормализация входящих свечей в `signal_candleUpdated` / `signal_candleClosed`.

### Indicators

- `IndicatorService`
- `IndicatorEngine`
- `EMA`
- `RSI`
- `ATR`
- `Donchian`
- `IndicatorDialog`

Сейчас торговый контур уже не пересчитывает индикаторы на каждый intrabar tick. UI может перестраивать overlay-линии для отображения, но hot path постепенно отделяется.

### Trading Runtime

- `StrategyController`
- `StrategyRunner`
- `IStrategy`
- `StrategyFactory`
- `RiskManager`
- `DemoExecutionService`
- `TradeJournal`
- `TradesModel`

Текущий поток:

```text
MarketDataService
  -> StrategyRunner
  -> IStrategy
  -> RiskManager
  -> DemoExecutionService
  -> TradeJournal
  -> TradesModel/UI callback
```

`TradeJournal` уже владеет своим состоянием и больше не зависит напрямую от `TradesModel`. UI получает изменения через callbacks.

### Backtest

- `BacktestController`
- `BacktestMarketDataService`
- `BacktestEngine`
- `BacktestTypes`
- `BacktestTradesModel`
- `BacktestWidget`
- `BacktestReportExporter`

Backtest умеет строить equity / drawdown / PnL graphs, таблицу сделок и сохранять отчеты в JSON / CSV.

### Core Events / Latency

Подготовительный слой для будущей event-driven архитектуры:

- `MarketEvent`
- `SignalEvent`
- `OrderEvent`
- `FillEvent`
- `TradeEvent`
- `LatencyTimestamp`
- `LatencyStats`
- `LatencyCollector`

Очереди и потоки пока специально не добавляются. Сначала фиксируются границы данных и baseline latency.

---

## UML / Architecture Docs

Единственный формат диаграмм в проекте — **PlantUML**.

Файлы:

- `TradeSoft UML/TradeSoft_Architecture.puml` — high-level class/component view
- `TradeSoft UML/TradeSoft_MarketDataFlow.puml` — market data sequence
- `TradeSoft UML/TradeSoft_TradingFlow.puml` — trading runtime sequence
- `TradeSoft UML/TradeSoft_BacktestAndLatency.puml` — backtest + low-latency preparation

PNG для README:

<img src="TradeSoft%20UML/TradeSoft_Architecture.png" alt="TradeSoft Architecture" width="100%">

<img src="TradeSoft%20UML/TradeSoft_MarketDataFlow.png" alt="TradeSoft Market Data Flow" width="100%">

<img src="TradeSoft%20UML/TradeSoft_TradingFlow.png" alt="TradeSoft Trading Flow" width="100%">

<img src="TradeSoft%20UML/TradeSoft_BacktestAndLatency.png" alt="TradeSoft Backtest And Latency" width="100%">

Сгенерировать SVG:

```bash
PLANTUML_LIMIT_SIZE=16384 plantuml -tsvg "TradeSoft UML"/*.puml
```

Сгенерировать PNG:

```bash
PLANTUML_LIMIT_SIZE=16384 plantuml -tpng "TradeSoft UML"/*.puml
```

---

## Roadmap

### Этап 1 — Основа приложения

- [x] Базовая структура проекта
- [x] Главное окно
- [x] Виджет графика
- [x] Таймфреймы
- [x] Выбор инструмента
- [x] Подключение источника данных
- [x] Базовая доменная модель
- [x] Разделение на UI / core / service / exchange / domain

### Этап 2 — Market Data

- [x] Загрузка исторических свечей
- [x] Подключение биржевого клиента
- [x] Cache fallback
- [x] Realtime polling
- [x] WebSocket realtime foundation
- [x] Устойчивость realtime при ошибках
- [ ] Более строгая унификация REST / WebSocket events

### Этап 3 — Аналитика и визуализация

- [x] EMA
- [x] RSI
- [x] ATR
- [x] Donchian / ценовые каналы
- [x] Отображение индикаторов на графике
- [x] Визуализация торговых уровней
- [ ] SMA
- [ ] MACD
- [ ] Объёмы
- [ ] Incremental indicators для trading hot path

### Этап 4 — Торговый контур

- [x] Базовая архитектура торгового контура
- [x] Strategy runner / controller
- [x] Risk manager
- [x] Demo / paper execution foundation
- [x] Базовое управление позицией
- [x] SL / TP логика
- [x] TradeJournal owns state
- [x] TradesModel подписан на callbacks
- [ ] Полноценный paper trading сценарий
- [ ] Расширенное управление позицией
- [ ] Полировка торгового UX

### Этап 5 — Стратегии и backtesting

- [x] Базовая архитектура стратегий
- [x] EMA Cross
- [x] EMA Pullback
- [x] EMA Scalp
- [x] Каркас backtest-модуля
- [x] Backtest engine
- [x] Backtest stats / graph models
- [x] JSON / CSV report exporter
- [ ] Больше сценариев проверки стратегий
- [ ] Intrabar execution model polish

### Этап 6 — Инфраструктура

- [x] CMake
- [x] CTest
- [x] GitHub Actions CI
- [x] QLoggingCategory
- [x] JSON config
- [x] App cache / reports folders
- [x] Unit tests

### Этап 7 — Low-Latency Refactor

- [x] Убрать очевидные расходы из hot path
- [x] Отвязать `TradeJournal` от `TradesModel`
- [x] Ввести core event structs
- [x] Добавить `LatencyTimestamp`
- [x] Добавить `LatencyStats` / `LatencyCollector`
- [ ] Подключить timestamps в текущий sync pipeline
- [ ] Перейти к внутреннему event pipeline
- [ ] Добавить thread boundaries
- [ ] Добавить SPSC / queue границы там, где ownership уже чистый

---

## Сборка на macOS

Установи Qt и CMake, затем собери проект из отдельной build-директории:

```bash
brew install cmake qt
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
open build/TradeSoft.app
```

Если Qt установлен не через Homebrew, передай путь к Qt в `CMAKE_PREFIX_PATH`, например `/Users/you/Qt/6.7.0/macos`.

### Тесты

Тесты включены в стандартную CMake-сборку:

```bash
cmake --build build --target TradeSoftTests
ctest --test-dir build --output-on-failure
```

### Логи

При запуске приложение пишет Qt-логи в пользовательскую папку приложения:

- macOS: `~/Library/Application Support/BortonGo/TradeSoft/tradesoft.log`

### Конфигурация

При первом запуске создаётся JSON-конфиг:

- macOS: `~/Library/Application Support/BortonGo/TradeSoft/config.json`

В нём можно менять дефолтный инструмент, таймфрейм, realtime transport, интервал polling и список символов.

### Кэш свечей

Свечи для графика кэшируются локально:

- macOS: `~/Library/Caches/BortonGo/TradeSoft/marketdata`

### Backtest Reports

После запуска backtest приложение сохраняет последние результаты:

- summary JSON: `~/Library/Application Support/BortonGo/TradeSoft/reports/latest_backtest_summary.json`
- trades CSV: `~/Library/Application Support/BortonGo/TradeSoft/reports/latest_backtest_trades.csv`

Также сохраняются timestamped snapshots рядом с `latest_*`, чтобы история прогонов не перезаписывалась.

---

## Скриншоты

<img width="1909" height="978" alt="image" src="https://github.com/user-attachments/assets/fc4b5ac7-dc64-4def-9559-f379c5f0ffcb" />

<img width="1908" height="980" alt="image" src="https://github.com/user-attachments/assets/b79d4d67-9f8b-40db-828b-cfbf8ca1526e" />

<img width="1910" height="975" alt="image" src="https://github.com/user-attachments/assets/1c5c1ec5-845c-4345-8d4e-497318033a17" />

---

## Статус проекта

Проект находится в активной разработке. Сейчас основной фокус — довести trading runtime, backtest и low-latency refactor до состояния, где hot path можно измерять и постепенно отделять от UI.

README и UML обновляются вместе с развитием проекта.

---

## Автор

BortonGo

---

## Лицензия

MIT License. Подробнее см. [LICENSE](LICENSE).
