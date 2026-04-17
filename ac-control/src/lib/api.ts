import type { ACMode, ACState, FanSpeed, Zone } from '@/types/ac'

type AcApiMode = 1 | 2 | 3 | 4 | 5
type AcApiFanSpeed = 1 | 2 | 3

export interface AcApiState {
  onOff: boolean
  mode: AcApiMode
  fanSpeed: AcApiFanSpeed
  currentTemp?: number
  setTemp: number
  zone0: boolean
  zone1: boolean
  zone2: boolean
  zone3: boolean
  zone4: boolean
  zone5: boolean
}

const MODE_TO_API: Record<ACMode, AcApiMode> = {
  cool: 1,
  heat: 2,
  vent: 3,
  dry: 4,
  auto: 5,
}

const MODE_FROM_API: Record<AcApiMode, ACMode> = {
  1: 'cool',
  2: 'heat',
  3: 'vent',
  4: 'dry',
  5: 'auto',
}

const FAN_SPEED_TO_API: Record<FanSpeed, AcApiFanSpeed> = {
  low: 1,
  medium: 2,
  high: 3,
}

const FAN_SPEED_FROM_API: Record<AcApiFanSpeed, FanSpeed> = {
  1: 'low',
  2: 'medium',
  3: 'high',
}

const ZONE_COUNT = 6

// Placeholder for now; this can be replaced by an env var later.
export const API_BASE_URL = 'http://localhost:5000'

const API_URL = new URL('/api', API_BASE_URL).href

function createDefaultZones(): Zone[] {
  return Array.from({ length: ZONE_COUNT }, (_, i) => ({
    id: i + 1,
    name: `Zone ${i + 1}`,
    enabled: true,
  }))
}

export function fromApiState(apiState: AcApiState): ACState {
  const zones = createDefaultZones()
  const zoneValues = [
    apiState.zone0,
    apiState.zone1,
    apiState.zone2,
    apiState.zone3,
    apiState.zone4,
    apiState.zone5,
  ]

  return {
    power: apiState.onOff,
    currentTemperature: apiState.currentTemp ?? apiState.setTemp,
    desiredTemperature: apiState.setTemp,
    mode: MODE_FROM_API[apiState.mode],
    fanSpeed: FAN_SPEED_FROM_API[apiState.fanSpeed],
    zones: zones.map((zone, i) => ({ ...zone, enabled: zoneValues[i] })),
  }
}

export function toApiState(state: ACState): AcApiState {
  const zoneValues = Array.from({ length: ZONE_COUNT }, (_, i) => {
    return state.zones.find((zone) => zone.id === i + 1)?.enabled ?? false
  })

  return {
    onOff: state.power,
    mode: MODE_TO_API[state.mode],
    fanSpeed: FAN_SPEED_TO_API[state.fanSpeed],
    currentTemp: state.currentTemperature,
    setTemp: state.desiredTemperature,
    zone0: zoneValues[0],
    zone1: zoneValues[1],
    zone2: zoneValues[2],
    zone3: zoneValues[3],
    zone4: zoneValues[4],
    zone5: zoneValues[5],
  }
}

export async function fetchAcState(): Promise<ACState> {
  const response = await fetch(API_URL)

  if (!response.ok) {
    throw new Error(`Failed to fetch AC state (${response.status})`)
  }

  const data = (await response.json()) as AcApiState
  return fromApiState(data)
}

export async function postAcState(state: ACState): Promise<void> {
  const payload: AcApiState = {
    ...toApiState(state),
    currentTemp: undefined,
  }

  const response = await fetch(API_URL, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(payload),
  })

  if (!response.ok) {
    throw new Error(`Failed to update AC state (${response.status})`)
  }
}

export function createAcWebSocket(onMessage: (state: ACState) => void): WebSocket {
  const baseUrl = new URL(API_BASE_URL)
  const protocol = baseUrl.protocol === 'https:' ? 'wss:' : 'ws:'
  const wsAddress = `${protocol}//${baseUrl.host}/ws`
  const socket = new WebSocket(wsAddress)

  socket.onmessage = (event) => {
    const data = JSON.parse(event.data) as AcApiState
    onMessage(fromApiState(data))
  }

  return socket
}
