# Тест-план для UDP монитора

## Объект тестирования
Программа на C++ (grpc_udp_monitor)

## Проверяемые сценарии

| Тест | Ожидаемый результат |
|------|---------------------|
| test_empty | packets=0, aBytes=0 |
| test_one_datagram_no_A | packets=1, aBytes=0 |
| test_one_datagram_with_A | packets=1, aBytes=3 |
| test_multiple_datagrams | packets=3, aBytes=5 |

## Тайм-ауты
- TIMEOUT_READY = 5 секунд
- TIMEOUT_SHUTDOWN = 2 секунды
