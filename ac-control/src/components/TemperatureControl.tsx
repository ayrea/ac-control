import { ChevronDown, ChevronUp } from 'lucide-react'

import { Button } from '@/components/ui/button'
import { cn } from '@/lib/utils'

type TemperatureControlProps = {
  desiredTemperature: number
  minTemp: number
  maxTemp: number
  power: boolean
  onIncrement: () => void
  onDecrement: () => void
}

export function TemperatureControl({
  desiredTemperature,
  minTemp,
  maxTemp,
  power,
  onIncrement,
  onDecrement,
}: TemperatureControlProps) {
  const atMin = desiredTemperature <= minTemp
  const atMax = desiredTemperature >= maxTemp

  return (
    <div
      className={'flex items-center justify-center gap-4'}
    >
      <Button
        type="button"
        variant="secondary"
        size="icon-lg"
        className="min-h-12 min-w-12 rounded-full"
        onClick={onDecrement}
        disabled={atMin}
        aria-label="Decrease target temperature by 1 degree"
      >
        <ChevronDown className="size-7" aria-hidden />
      </Button>
      <div className="flex min-w-[5.5rem] flex-col items-center">
        <span className="text-muted-foreground text-xs font-medium tracking-wide uppercase">
          Set temp
        </span>
        <span
          className={cn(
            'text-4xl font-semibold tabular-nums',
            !power && 'text-muted-foreground',
          )}
        >
          {desiredTemperature}
          <span className="text-2xl font-normal text-muted-foreground">°C</span>
        </span>
      </div>
      <Button
        type="button"
        variant="secondary"
        size="icon-lg"
        className="min-h-12 min-w-12 rounded-full"
        onClick={onIncrement}
        disabled={atMax}
        aria-label="Increase target temperature by 1 degree"
      >
        <ChevronUp className="size-7" aria-hidden />
      </Button>
    </div>
  )
}
