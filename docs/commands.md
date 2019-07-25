An attempt to document the bettle commands and responses.


## Requests (all modes)

- `deb <n>`

  set the debug level (2 is currently max) for logging

  response: `deb <status#>`

- `mod (cen|per)`

  switch modes:
    - central: hub
    - peripheral: bluetooth LE peripheral with attributes (for wifi setup)

  response: `mod <status#>`


## Requests (central mode)

- `shh`

  toggle "quiet mode", which hides `adv` messages -- these can be a flood
  sometimes when operating beetle manually

- `con <periph_id>`

  connect to a peripheral by hardware ID (colon-delimited hex)

  response: `con <periph_id> <status#> [fd#]`

- `can <periph_id>`

  cancel a connection request to a peripheral (if it hasn't connected yet)

  response: `can <periph_id> <status#>`

- `dis <fd#>`

  disconnect a peripheral

- `kat <fd#>`

  response: `kat <fd#> <status#> <attr#> [flags#]`

- `wri <fd#> <attr#> <hex>`

  response: `wri <fd#> <attr#> <status#>`

- `rea <fd#> <attr#>`

  response: `rea <fd#> <attr#> <status#> [hex]`

- `nen <fd#> <attr#> <enable? 1>`

  enable notifications

  response: `nen <fd#> <attr#> <status#>`


## Requests (peripheral mode)

- `pad <flags#> <appearance#> <hex>`

  response: `pad <status#>`

- `pin <uuid#> <hex>`

  peripheral indicate

  response: `pin <uuid#> <status#>`

- `pka <attr#> <flags#>`

  response: `pka <attr#> <flags#> <status#>`

- `pdi`

  disconnect any bluetooth peer (hub)

  response: `pdi [addr]`


## Events (central mode)

- `adv <periph_id> <manufacturer_data_hex> <rssi#>`

  advertisement from a BLE device

- `dis <fd#> <status#> [reason#]`

  peripheral disconnected (possibly by request, possibly spuriously)

- `not <fd#> <attr#> <hex>`

- `shh <value>`

  1 if quiet mode is active, 0 if not


## Events (peripheral mode)

- `pne <uuid#> <enable? 1>`

  notification request for a characteristic

- `pwr <uuid#> <hex>`

  write to a characteristic

- `pco <addr>`

  connection from hub

- `pdi <addr>`

  disconnected


## Status codes

- 0: OK
- 1: Unknown connection
- 2: Unknown attribute
- 3: Pending
- 4: Not permitted
- 5: I/O error
- 6: Unknown device
- 7: Already connected
- 8: Connection list is full
- 9: Timeout
- 10: L2Cap connection failed
- 11: Not connected
- 12: Bluetooth error
- 13: Bad parameter
- 14: Already in mode
- 15: Wrong mode
- 16: Database is full
- 17: Unknown command
