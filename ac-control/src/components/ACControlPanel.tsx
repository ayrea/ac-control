import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from '@/components/ui/card'
import { useACControl } from '@/hooks/useACControl'

import { FanSpeedSelector } from './FanSpeedSelector'
import { ModeSelector } from './ModeSelector'
import { PowerButton } from './PowerButton'
import { TemperatureControl } from './TemperatureControl'
import { TemperatureDisplay } from './TemperatureDisplay'
import { ZoneControl } from './ZoneControl'

export function ACControlPanel() {
  const {
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
    minTemp,
    maxTemp,
  } = useACControl()

  return (
    <Card className="border-border/80 bg-card/95 w-full max-w-md shadow-lg backdrop-blur-sm">
      <CardHeader className="gap-1 pb-2">
        <div className="flex items-start justify-between gap-3">
          <div>
            <CardTitle className="text-xl">Climate</CardTitle>
            <CardDescription>Air conditioning</CardDescription>
          </div>
          <PowerButton power={power} onToggle={togglePower} />
        </div>
      </CardHeader>
      <CardContent className="flex flex-col gap-6 pt-0">
        <TemperatureDisplay
          currentTemperature={currentTemperature}
        />
        <TemperatureControl
          desiredTemperature={desiredTemperature}
          minTemp={minTemp}
          maxTemp={maxTemp}
          power={power}
          onIncrement={incrementTemp}
          onDecrement={decrementTemp}
        />
        <ModeSelector mode={mode} power={power} onModeChange={setMode} />
        <FanSpeedSelector
          fanSpeed={fanSpeed}
          power={power}
          onFanSpeedChange={setFanSpeed}
        />
        <ZoneControl zones={zones} onToggleZone={toggleZone} />
      </CardContent>
    </Card>
  )
}
