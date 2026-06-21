# REST API Documentation - Moonshine Cooler

## Базовый URL

```
http://<ESP8266_IP>:<PORT>
http://192.168.1.100  (порт по умолчанию 80)
```

## Общая информация

- **Содержимое:** JSON
- **Кодировка:** UTF-8
- **Формат даты/времени:** Unix timestamp (миллисекунды)

## Endpoints

### 1. GET /api/status

Получить полный статус системы.

**Запрос:**
```http
GET /api/status HTTP/1.1
Host: 192.168.1.100
```

**Ответ (200 OK):**
```json
{
  "temp_liquid": 28.5,
  "temp_vapor": 42.3,
  "compressor_on": true,
  "fan_on": false,
  "auto_mode": true,
  "target_temp": 25.0,
  "timestamp": 1687123456789
}
```

**Поля ответа:**
| Поле | Тип | Описание |
|---|---|---|
| `temp_liquid` | float | Температура охлаждающей жидкости (°C) |
| `temp_vapor` | float | Температура паров (°C) |
| `compressor_on` | boolean | Компрессор включен |
| `fan_on` | boolean | Вентилятор включен |
| `auto_mode` | boolean | Автоматический режим активен |
| `target_temp` | float | Целевая температура (°C) |
| `timestamp` | integer | Время последнего обновления |

---

### 2. POST /api/compressor

Управление компрессором (включить/выключить).

**Запрос:**
```http
POST /api/compressor HTTP/1.1
Host: 192.168.1.100
Content-Type: application/json

{
  "on": true
}
```

**Параметры:**
| Параметр | Тип | Требуется | Описание |
|---|---|---|---|
| `on` | boolean | ✓ | true - включить, false - выключить |

**Ответ (200 OK):**
```json
{
  "success": true,
  "compressor_on": true
}
```

**Коды ошибок:**
- `400` - Invalid JSON
- `500` - Server error

**Примеры cURL:**
```bash
# Включить
curl -X POST http://192.168.1.100/api/compressor \
  -H "Content-Type: application/json" \
  -d '{"on": true}'

# Выключить
curl -X POST http://192.168.1.100/api/compressor \
  -H "Content-Type: application/json" \
  -d '{"on": false}'
```

---

### 3. POST /api/fan

Управление вентилятором.

**Запрос:**
```http
POST /api/fan HTTP/1.1
Host: 192.168.1.100
Content-Type: application/json

{
  "on": true
}
```

**Параметры:**
| Параметр | Тип | Требуется | Описание |
|---|---|---|---|
| `on` | boolean | ✓ | true - включить, false - выключить |

**Ответ (200 OK):**
```json
{
  "success": true,
  "fan_on": true
}
```

---

### 4. POST /api/mode

Переключить режим работы (автоматический/ручной).

**Запрос:**
```http
POST /api/mode HTTP/1.1
Host: 192.168.1.100
Content-Type: application/json

{
  "auto": true
}
```

**Параметры:**
| Параметр | Тип | Требуется | Описание |
|---|---|---|---|
| `auto` | boolean | ✓ | true - автоматический, false - ручной |

**Ответ (200 OK):**
```json
{
  "success": true,
  "auto_mode": true
}
```

**Примечание:** При переключении в ручной режим компрессор и вентилятор автоматически выключаются.

---

### 5. POST /api/temperature

Установить целевую температуру для автоматического режима.

**Запрос:**
```http
POST /api/temperature HTTP/1.1
Host: 192.168.1.100
Content-Type: application/json

{
  "target_temp": 25.0
}
```

**Параметры:**
| Параметр | Тип | Требуется | Диапазон | Описание |
|---|---|---|---|---|
| `target_temp` | float | ✓ | 0-100 | Целевая температура (°C) |

**Ответ (200 OK):**
```json
{
  "success": true,
  "target_temp": 25.0
}
```

**Ошибки:**
```json
{
  "error": "Temperature out of range"
}
```

**Примеры:**
```bash
# Установить 22°C
curl -X POST http://192.168.1.100/api/temperature \
  -H "Content-Type: application/json" \
  -d '{"target_temp": 22.0}'

# Установить 30°C
curl -X POST http://192.168.1.100/api/temperature \
  -H "Content-Type: application/json" \
  -d '{"target_temp": 30.0}'
```

---

### 6. GET /

Получить веб-интерфейс (HTML).

**Запрос:**
```http
GET / HTTP/1.1
Host: 192.168.1.100
```

**Ответ:** HTML страница веб-интерфейса

---

### 7. GET /css/style.css

Получить CSS стили.

**Запрос:**
```http
GET /css/style.css HTTP/1.1
Host: 192.168.1.100
```

---

### 8. GET /js/app.js

Получить JavaScript приложение.

**Запрос:**
```http
GET /js/app.js HTTP/1.1
Host: 192.168.1.100
```

---

## Примеры использования

### JavaScript (Fetch API)

```javascript
// Получить статус
fetch('http://192.168.1.100/api/status')
  .then(response => response.json())
  .then(data => console.log(data));

// Включить компрессор
fetch('http://192.168.1.100/api/compressor', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ on: true })
})
  .then(response => response.json())
  .then(data => console.log(data));

// Установить режим автомат и целевую температуру
fetch('http://192.168.1.100/api/mode', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ auto: true })
});

fetch('http://192.168.1.100/api/temperature', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ target_temp: 24.5 })
});
```

### Python (requests)

```python
import requests
import json

BASE_URL = "http://192.168.1.100"

# Получить статус
response = requests.get(f"{BASE_URL}/api/status")
print(response.json())

# Включить компрессор
response = requests.post(
    f"{BASE_URL}/api/compressor",
    json={"on": True},
    headers={"Content-Type": "application/json"}
)
print(response.json())

# Установить целевую температуру
response = requests.post(
    f"{BASE_URL}/api/temperature",
    json={"target_temp": 23.0},
    headers={"Content-Type": "application/json"}
)
print(response.json())
```

### cURL

```bash
# Получить статус
curl http://192.168.1.100/api/status

# Включить компрессор
curl -X POST http://192.168.1.100/api/compressor \
  -H "Content-Type: application/json" \
  -d '{"on": true}'

# Выключить вентилятор
curl -X POST http://192.168.1.100/api/fan \
  -H "Content-Type: application/json" \
  -d '{"on": false}'

# Переключиться в автоматический режим
curl -X POST http://192.168.1.100/api/mode \
  -H "Content-Type: application/json" \
  -d '{"auto": true}'

# Установить целевую температуру 26.5°C
curl -X POST http://192.168.1.100/api/temperature \
  -H "Content-Type: application/json" \
  -d '{"target_temp": 26.5}'
```

---

## Обработка ошибок

### Возможные коды ответов

| Код | Описание |
|---|---|
| `200` | OK - Успешный запрос |
| `400` | Bad Request - Некорректный JSON |
| `404` | Not Found - Endpoint не найден |
| `500` | Internal Server Error - Ошибка сервера |

### Примеры ошибок

```json
{
  "error": "Invalid JSON"
}
```

```json
{
  "error": "Temperature out of range"
}
```

---

## Ограничения и рекомендации

1. **Частота обновления:** Не более 1 запроса в 100ms для статуса
2. **Таймаут:** Используйте таймаут 5-10 секунд для HTTP запросов
3. **Подсоединение:** Убедитесь, что устройство в одной сети с ESP8266
4. **IP адрес:** Может измениться после перезагрузки (используйте DHCP статическое назначение)

---

## Автоматическое управление (Примеры сценариев)

### Сценарий 1: Поддержание температуры 22-25°C в режиме автомат

```javascript
async function setupAutoMode() {
  // Переключиться в автоматический режим
  await fetch('http://192.168.1.100/api/mode', {
    method: 'POST',
    body: JSON.stringify({ auto: true })
  });
  
  // Установить целевую температуру
  await fetch('http://192.168.1.100/api/temperature', {
    method: 'POST',
    body: JSON.stringify({ target_temp: 23.5 })
  });
}

setupAutoMode();
```

### Сценарий 2: Полный цикл охлаждения

```javascript
async function coolDown() {
  // Включить вентилятор
  await fetch('http://192.168.1.100/api/fan', {
    method: 'POST',
    body: JSON.stringify({ on: true })
  });
  
  // Включить компрессор
  await fetch('http://192.168.1.100/api/compressor', {
    method: 'POST',
    body: JSON.stringify({ on: true })
  });
  
  // Ждать 30 секунд
  await new Promise(resolve => setTimeout(resolve, 30000));
  
  // Выключить компрессор
  await fetch('http://192.168.1.100/api/compressor', {
    method: 'POST',
    body: JSON.stringify({ on: false })
  });
}

coolDown();
```
