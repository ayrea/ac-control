import { useCallback, useState } from 'react'

import type { ACMode, FanSpeed, Zone } from '@/types/ac'

const MIN_TEMP = 16
const MAX_TEMP = 30

const initialZones: Zone[] = Array.from({ length: 6 }, (_, i) => ({
  id: i + 1,
  name: `Zone ${i + 1}`,
  enabled: true,
}))

export function useACControl() {
  const [power, setPower] = useState(true)
  const [currentTemperature] = useState(24)
  const [desiredTemperature, setDesiredTemperature] = useState(22)
  const [mode, setMode] = useState<ACMode>('cool')
  const [fanSpeed, setFanSpeed] = useState<FanSpeed>('medium')
  const [zones, setZones] = useState<Zone[]>(initialZones)

  const togglePower = useCallback(() => {
    setPower((p) => !p)
  }, [])

  const incrementTemp = useCallback(() => {
    setDesiredTemperature((t) => Math.min(MAX_TEMP, t + 1))
  }, [])

  const decrementTemp = useCallback(() => {
    setDesiredTemperature((t) => Math.max(MIN_TEMP, t - 1))
  }, [])

  const toggleZone = useCallback((zoneId: number) => {
    setZones((prev) =>
      prev.map((z) => (z.id === zoneId ? { ...z, enabled: !z.enabled } : z)),
    )
  }, [])

  return {
    power,
    currentTemperature,
    desiredTemperature,
    mode,
    fanSpeed,
    zones,
    togglePower,
    incrementTemp,
    decrementTemp,
    setMode,
    setFanSpeed,
    toggleZone,
    minTemp: MIN_TEMP,
    maxTemp: MAX_TEMP,
  }
}
