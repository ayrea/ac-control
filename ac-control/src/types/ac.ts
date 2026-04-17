export type ACMode = 'cool' | 'heat' | 'vent' | 'dry' | 'auto'

export type FanSpeed = 'low' | 'medium' | 'high'

export interface Zone {
  id: number
  name: string
  enabled: boolean
}

export interface ACState {
  power: boolean
  currentTemperature: number
  desiredTemperature: number
  mode: ACMode
  fanSpeed: FanSpeed
  zones: Zone[]
}
