import { useCallback, useEffect, useState } from 'react'

import {
  createAcWebSocket,
  fetchAcState,
  postAcState,
} from '@/lib/api'
import type { ACMode, ACState, FanSpeed, Zone } from '@/types/ac'

const MIN_TEMP = 16
const MAX_TEMP = 30

const initialZones: Zone[] = Array.from({ length: 6 }, (_, i) => ({
  id: i + 1,
  name: `Zone ${i + 1}`,
  enabled: true,
}))

export function useACControl() {
  const [acState, setAcState] = useState<ACState>({
    power: false,
    currentTemperature: 24,
    desiredTemperature: 22,
    mode: 'cool',
    fanSpeed: 'medium',
    zones: initialZones,
  })

  const syncAndPersist = useCallback((updater: (prev: ACState) => ACState) => {
    setAcState((prev) => {
      const next = updater(prev)
      void postAcState(next).catch((error: unknown) => {
        console.error('Failed to post AC state', error)
      })
      return next
    })
  }, [])

  useEffect(() => {
    let active = true

    void fetchAcState()
      .then((state) => {
        if (active) setAcState(state)
      })
      .catch((error: unknown) => {
        console.error('Failed to fetch AC state', error)
      })

    const socket = createAcWebSocket((state) => {
      if (active) setAcState(state)
    })

    return () => {
      active = false
      socket.close()
    }
  }, [])

  const togglePower = useCallback(() => {
    syncAndPersist((prev) => ({ ...prev, power: !prev.power }))
  }, [syncAndPersist])

  const incrementTemp = useCallback(() => {
    syncAndPersist((prev) => ({
      ...prev,
      desiredTemperature: Math.min(MAX_TEMP, prev.desiredTemperature + 1),
    }))
  }, [syncAndPersist])

  const decrementTemp = useCallback(() => {
    syncAndPersist((prev) => ({
      ...prev,
      desiredTemperature: Math.max(MIN_TEMP, prev.desiredTemperature - 1),
    }))
  }, [syncAndPersist])

  const setMode = useCallback(
    (mode: ACMode) => {
      syncAndPersist((prev) => ({ ...prev, mode }))
    },
    [syncAndPersist],
  )

  const setFanSpeed = useCallback(
    (fanSpeed: FanSpeed) => {
      syncAndPersist((prev) => ({ ...prev, fanSpeed }))
    },
    [syncAndPersist],
  )

  const toggleZone = useCallback((zoneId: number) => {
    syncAndPersist((prev) => ({
      ...prev,
      zones: prev.zones.map((zone) =>
        zone.id === zoneId ? { ...zone, enabled: !zone.enabled } : zone,
      ),
    }))
  }, [syncAndPersist])

  return {
    power: acState.power,
    currentTemperature: acState.currentTemperature,
    desiredTemperature: acState.desiredTemperature,
    mode: acState.mode,
    fanSpeed: acState.fanSpeed,
    zones: acState.zones,
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
