import { Thermometer } from 'lucide-react'

type TemperatureDisplayProps = {
  currentTemperature: number
}

export function TemperatureDisplay({
  currentTemperature
}: TemperatureDisplayProps) {
  return (
    <div className="flex flex-col items-center gap-1 text-center">
      <p className="text-muted-foreground flex items-center gap-1.5 text-xs font-medium tracking-wide uppercase">
        <Thermometer className="size-3.5 shrink-0" aria-hidden />
        Current
      </p>
      <p
        className="text-5xl font-semibold tabular-nums tracking-tight sm:text-6xl"
        aria-live="polite"
      >
        {currentTemperature.toFixed(1)}
        <span className="text-3xl font-normal text-muted-foreground sm:text-4xl">
          °C
        </span>
      </p>
    </div>
  )
}
