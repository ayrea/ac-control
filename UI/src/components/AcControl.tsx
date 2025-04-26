import Box from '@mui/material/Box';
import Paper from '@mui/material/Paper';
import { Button, Grid, MenuItem, Select, SelectChangeEvent, SvgIcon, Switch, Typography } from '@mui/material';
import { ReactElement, useState } from 'react';
import PowerSettingsNewIcon from '@mui/icons-material/PowerSettingsNew';
import ArrowDropUpIcon from '@mui/icons-material/ArrowDropUp';
import ArrowDropDownIcon from '@mui/icons-material/ArrowDropDown';
import { getEnumKeys } from '../helperFunctions';
import AcUnitIcon from '@mui/icons-material/AcUnit';
import SunnyIcon from '@mui/icons-material/Sunny';
import CloudQueueIcon from '@mui/icons-material/CloudQueue';
import BeachAccessIcon from '@mui/icons-material/BeachAccess';
import AutoModeIcon from '@mui/icons-material/AutoMode';

interface AcMode {
    index: number;
    icon: ReactElement;
    label: string
}

const acModes: AcMode[] = [
    {
        index: 0,
        label: "Cool",
        icon: <AcUnitIcon sx={{ mt: 1, color: "blue" }} />
    },
    {
        index: 1,
        label: "Heat",
        icon: <SunnyIcon sx={{ mt: 1, color: "red" }} />
    },
    {
        index: 2,
        label: "Vent",
        icon: <CloudQueueIcon sx={{ mt: 1, color: "black" }} />
    },
    {
        index: 3,
        label: "Dry",
        icon: <BeachAccessIcon sx={{ mt: 1, color: "#adad00" }} />
    },
    {
        index: 4,
        label: "Auto",
        icon: <AutoModeIcon sx={{ mt: 1, color: "black" }} />
    }
];

enum FanSpeedEnum {
    Low,
    Medium,
    High
}

interface AcState {
    onOff: boolean,
    mode: AcMode,
    fanSpeed: FanSpeedEnum,
    currentTemp: number,
    setTemp: number
    zones: AcZone[]
}

interface AcZone {
    index: number,
    label: string,
    isOpen: boolean
}

const acInitialState = (): AcState => {
    return {
        onOff: false,
        mode: acModes[0],
        fanSpeed: FanSpeedEnum.Low,
        currentTemp: 24.3,
        setTemp: 25,
        zones: [
            {
                index: 0,
                label: "Zone 1",
                isOpen: true
            },
            {
                index: 1,
                label: "Zone 2",
                isOpen: true
            },
            {
                index: 2,
                label: "Zone 3",
                isOpen: true
            },
            {
                index: 3,
                label: "zone 4",
                isOpen: true
            },
            {
                index: 4,
                label: "Zone 5",
                isOpen: true
            },
            {
                index: 5,
                label: "Zone 6",
                isOpen: true
            },
        ]
    }
}

export default function AcControl() {
    const [acState, setAcState] = useState<AcState>(acInitialState);

    const togglePower = () => {
        setAcState({ ...acState, onOff: !acState.onOff })
    }

    const temperatureUpDown = (e: any) => {
        let currentSetTemp = acState.setTemp;
        if (e.currentTarget.id === "tempDown") {
            currentSetTemp -= 1;
            if (currentSetTemp < 15) {
                currentSetTemp = 15;
            }
        }
        else {
            currentSetTemp += 1;
            if (currentSetTemp > 30) {
                currentSetTemp = 30;
            }
        }

        setAcState({ ...acState, setTemp: currentSetTemp });
    }

    const handleChangeMode = (e: SelectChangeEvent) => {
        const selectedMode: AcMode = acModes.find((mode) => mode.index.toString() === e.target.value.toString()) ?? acModes[0];
        setAcState({ ...acState, mode: { ...selectedMode } });
    }

    const handleChangeFanSpeed = (e: SelectChangeEvent) => {
        const newValue: number = Number.parseInt(e.target.value);
        setAcState({ ...acState, fanSpeed: newValue });
    }

    const handleZoneChanged = (e: React.ChangeEvent<HTMLInputElement>) => {
        const index: number = Number.parseInt(e.target.id);
        const isOpen: boolean = e.target.checked;

        const newZones = [...acState.zones];
        newZones[index].isOpen = isOpen;
        setAcState({ ...acState, zones: newZones });
    }

    return (
        <Paper elevation={6} sx={{ maxWidth: "560px" }}>
            <Box height={"4px"}>
            </Box>
            <Box m={1} pl={2} bgcolor={"lightgrey"} justifyItems={'left'} borderRadius={3} sx={{ bgcolor: "#d7e4ef" }}>
                <Typography variant='h4'>AC Control</Typography>
            </Box>
            <Grid container spacing={1}>
                <Grid size={4} justifyItems={'right'} />
                <Grid size={8} justifyItems={'left'} sx={{ mb: 2 }}>
                    <Box m={1} mt={1} display={"flex"}>
                        <Button variant='contained' sx={acState.onOff ? { bgcolor: "#00c200" } : { bgcolor: "grey" }} onClick={togglePower}><PowerSettingsNewIcon fontSize='large' /></Button>
                        <Typography variant='h4' mx={4} mt={0.5}>{acState.onOff ? "On" : "Off"}</Typography>
                    </Box>
                </Grid>
                <Grid size={5} justifyItems={'right'}>
                    <Typography variant='h6' mt={"4px"}>Current Temp:</Typography>
                </Grid>
                <Grid size={7} mb={1} justifyItems={'left'}>
                    <Typography ml={1} variant='h4'>{acState.currentTemp.toFixed(1)}</Typography>
                </Grid>

                <Grid size={4} justifyItems={'right'}>
                    <Typography variant='h6' mt={2}>Set Temp:</Typography>
                </Grid>
                <Grid size={8} justifyItems={'left'} sx={{ mb: 1 }}>
                    <Box m={1} mt={1.5} display={"flex"}>
                        <Button id="tempDown" variant='contained' sx={{ width: "50px", height: "36px", bgcolor: "lightgray" }} onClick={temperatureUpDown}>
                            <SvgIcon sx={{ width: "50px", height: "50px", color: "black" }}><ArrowDropDownIcon fontSize='large' /></SvgIcon>
                        </Button>
                        <Typography variant='h4' mx={2}>{acState.setTemp.toFixed(1)}</Typography>
                        <Button id="tempUp" variant='contained' sx={{ width: "50px", height: "36px", bgcolor: "lightgray" }} onClick={temperatureUpDown}>
                            <SvgIcon sx={{ width: "50px", height: "50px", color: "black" }}><ArrowDropUpIcon fontSize='large' /></SvgIcon>
                        </Button>
                    </Box>
                </Grid>
                <Grid size={4} justifyItems={'right'}>
                    <Typography variant='h6' mt={2.5}>Mode:</Typography>
                </Grid>
                <Grid size={8} justifyItems={'left'} sx={{ mb: 1 }}>
                    <Box m={1} mt={1.5} display={"flex"}>
                        <Select sx={{ width: "230px", height: '46px' }} value={acState.mode.index.toString()} onChange={handleChangeMode}>
                            {acModes.map((mode) => {
                                return (
                                    <MenuItem key={mode.label} value={mode.index}>
                                        <Box display={"flex"}>
                                            {mode.icon}
                                            <Typography ml={2} variant='h4'>{mode.label}</Typography>
                                        </Box>
                                    </MenuItem>
                                );
                            })};
                        </Select>
                    </Box>
                </Grid>

                <Grid size={4} justifyItems={'right'}>
                    <Typography variant='h6' mt={2.5}>Fan Speed:</Typography>
                </Grid>
                <Grid size={8} justifyItems={'left'} sx={{ mb: 2 }}>
                    <Box m={1} mt={1.5} display={"flex"}>
                        <Select sx={{ width: "230px", height: '46px' }} value={acState.fanSpeed.toString()} onChange={handleChangeFanSpeed}>
                            {getEnumKeys(FanSpeedEnum).map((fanSpeed: string, index: number) => {
                                return (<MenuItem key={index} value={index}>
                                    <Typography variant='h4'>{fanSpeed}</Typography>
                                </MenuItem>);
                            })}
                        </Select>
                    </Box>
                </Grid>

                <Grid size={12} m={1} pl={2} mt={0} bgcolor={"lightgrey"} justifyItems={'left'} borderRadius={3} sx={{ bgcolor: "#d7e4ef" }}>
                    <Typography variant='h4'>Zones</Typography>
                </Grid>

                <Grid size={6} justifyItems={"right"} display={"flex"} flexDirection={"column"}>
                    {acState.zones.filter((zone, index) => index < 3 ? zone : null)
                        .map((zone, i) => {
                            return (<Grid size={12} key={"a" + i} justifyItems={"right"} display={"flex"} flexDirection={"row-reverse"}>
                                <Grid key={"a" + zone.index} size={4} sx={{ mb: 2 }} justifyItems={"right"}>
                                    <Switch id={i.toString()} key={zone.index} checked={zone.isOpen} onChange={handleZoneChanged}></Switch>
                                </Grid>
                                <Grid key={"b" + zone.index} size={8} justifyItems={"right"} >
                                    <Typography key={zone.index} variant='h6'>{zone.label}</Typography>
                                </Grid>
                            </Grid>
                            );
                        })
                    }
                </Grid>

                <Grid size={6} justifyItems={"right"} display={"flex"} flexDirection={"column"}>
                    {
                        acState.zones.filter((zone, index) => index >= 3 ? zone : null)
                            .map((zone) => {
                                return (<Grid size={12} key={zone.index} justifyItems={"right"} display={"flex"} flexDirection={"row-reverse"}>
                                    <Grid key={"zone-switch" + zone.index} size={5} sx={{ mb: 2 }} justifyItems={"right"}>
                                        <Switch id={zone.index.toString()} key={zone.index} checked={zone.isOpen} onChange={handleZoneChanged}></Switch>
                                    </Grid>
                                    <Grid size={7} key={zone.index} justifyItems={"right"} >
                                        <Typography key={zone.index} variant='h6'>{zone.label}</Typography>
                                    </Grid>
                                </Grid>
                                );
                            })
                    }
                </Grid>

            </Grid>
        </Paper>
    );
}
