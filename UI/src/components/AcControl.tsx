import Box from '@mui/material/Box';
import Paper from '@mui/material/Paper';
import { Button, Grid, MenuItem, Select, SelectChangeEvent, SvgIcon, Switch, Typography } from '@mui/material';
import { ReactElement, useEffect, useState } from 'react';
import PowerSettingsNewIcon from '@mui/icons-material/PowerSettingsNew';
import ArrowDropUpIcon from '@mui/icons-material/ArrowDropUp';
import ArrowDropDownIcon from '@mui/icons-material/ArrowDropDown';
import { getEnumKeys, getEnumValues } from '../helperFunctions';
import AcUnitIcon from '@mui/icons-material/AcUnit';
import SunnyIcon from '@mui/icons-material/Sunny';
import CloudQueueIcon from '@mui/icons-material/CloudQueue';
import BeachAccessIcon from '@mui/icons-material/BeachAccess';
import AutoModeIcon from '@mui/icons-material/AutoMode';
import acInitialState from '../data/initialState.json';

interface AcModeItem {
    index: number;
    icon: ReactElement;
    label: string
}

const acModes: AcModeItem[] = [
    {
        index: 1,
        label: "Cool",
        icon: <AcUnitIcon sx={{ mt: 1, color: "blue" }} />
    },
    {
        index: 2,
        label: "Heat",
        icon: <SunnyIcon sx={{ mt: 1, color: "red" }} />
    },
    {
        index: 3,
        label: "Vent",
        icon: <CloudQueueIcon sx={{ mt: 1, color: "black" }} />
    },
    {
        index: 4,
        label: "Dry",
        icon: <BeachAccessIcon sx={{ mt: 1, color: "#adad00" }} />
    },
    {
        index: 5,
        label: "Auto",
        icon: <AutoModeIcon sx={{ mt: 1, color: "black" }} />
    }
];

enum AcModeEnum {
    Cool = 1,
    Heat = 2,
    Vent = 3,
    Dry = 4,
    Auto = 5
}

enum FanSpeedEnum {
    Low = 1,
    Medium = 2,
    High = 3
}

interface AcState {
    onOff: boolean;
    mode: AcModeEnum;
    fanSpeed: FanSpeedEnum;
    currentTemp?: number;
    setTemp: number;
    zone0: boolean;
    zone1: boolean;
    zone2: boolean;
    zone3: boolean;
    zone4: boolean;
    zone5: boolean;
}

const zoneLabels: string[] = [
    "Zone 0",
    "Zone 1",
    "Zone 2",
    "Zone 3",
    "Zone 4",
    "Zone 5"
]

interface AcControlProps {
    baseApiUrl: string
}

export default function AcControl(props: AcControlProps) {
    const [acState, setAcState] = useState<AcState>(acInitialState);
    
    useEffect(() => {
        const wsAddress = props.baseApiUrl.replace("http://", "ws://") + "/ws";
        const ws = new WebSocket(wsAddress);

        ws.onmessage = (event) => {
            const data: AcState = JSON.parse(event.data);
            setAcState(data);
        }

        ws.onopen = () => console.log("WebSocket connected");
        ws.onclose = () => console.log("WebSocket disconnected");

        fetchAcState().then((x) => {
            setAcState(x);
        })
    }, []);

    async function fetchAcState(): Promise<AcState> {
        try {
            const url = new URL("/api", props.baseApiUrl).href;
            const response = await fetch(url);
            if (!response.ok) {
                console.error("Error fetching AcState", response.status, response.statusText);
                throw new Error("Error fetching AcState");
            }

            const data: AcState = await response.json();
            return data;
        }
        catch (error) {
            console.error("Error fetching AcState", error);
            throw error;
        }
    }

    async function sendPostRequest(url: string, data: string): Promise<void> {
        try {
          const response = await fetch(url, {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
            },
            body: data
          });
      
          if (!response.ok) {
            throw new Error(`Server responded with status ${response.status}`);
          }
        } catch (error) {
          console.error('Error sending POST request:', error);
        }
      }

    async function sendPost(newAcState: AcState) {
        setAcState(newAcState);

        const toSend = {...newAcState, currentTemp: undefined};
        const url = new URL("/api", props.baseApiUrl).href;
        sendPostRequest(url, JSON.stringify(toSend));
    }

    const togglePower = () => {
        sendPost({ ...acState, onOff: !acState.onOff });
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

        sendPost({ ...acState, setTemp: currentSetTemp });
    }

    const handleChangeMode = (e: SelectChangeEvent) => {
        sendPost({ ...acState, mode: Number.parseInt(e.target.value) });
    }

    const handleChangeFanSpeed = (e: SelectChangeEvent) => {
        const newValue: number = Number.parseInt(e.target.value);
        sendPost({ ...acState, fanSpeed: newValue });
    }

    const handleZoneChanged = (e: React.ChangeEvent<HTMLInputElement>) => {
        const zoneId: string = e.target.id;
        const isOpen: boolean = e.target.checked;

        switch (zoneId) {
            case "zone0":
                sendPost({ ...acState, zone0: isOpen });
                break;

            case "zone1":
                sendPost({ ...acState, zone1: isOpen });
                break;

            case "zone2":
                sendPost({ ...acState, zone2: isOpen });
                break;

            case "zone3":
                sendPost({ ...acState, zone3: isOpen });
                break;

            case "zone4":
                sendPost({ ...acState, zone4: isOpen });
                break;

            case "zone5":
                sendPost({ ...acState, zone5: isOpen });
                break;

            default:
                console.error('Invalid zone zoneId:', zoneId);
        }
    }

    const fanSpeedValues = getEnumValues(FanSpeedEnum);

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
                    <Typography ml={1} variant='h4'>{acState.currentTemp?.toFixed(1)}</Typography>
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
                        <Select sx={{ width: "230px", height: '46px' }} value={acState.mode.toString()} onChange={handleChangeMode}>
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
                                return (<MenuItem key={fanSpeedValues[index]} value={fanSpeedValues[index]}>
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
                    <Grid size={12} justifyItems={"right"} display={"flex"} flexDirection={"row-reverse"}>
                        <Grid size={4} sx={{ mb: 2 }} justifyItems={"right"}>
                            <Switch id="zone0" checked={acState.zone0} onChange={handleZoneChanged}></Switch>
                        </Grid>
                        <Grid size={8} justifyItems={"right"} >
                            <Typography variant='h6'>{zoneLabels[0]}</Typography>
                        </Grid>
                    </Grid>

                    <Grid size={12} justifyItems={"right"} display={"flex"} flexDirection={"row-reverse"}>
                        <Grid size={4} sx={{ mb: 2 }} justifyItems={"right"}>
                            <Switch id="zone1" checked={acState.zone1} onChange={handleZoneChanged}></Switch>
                        </Grid>
                        <Grid size={8} justifyItems={"right"} >
                            <Typography variant='h6'>{zoneLabels[1]}</Typography>
                        </Grid>
                    </Grid>

                    <Grid size={12} justifyItems={"right"} display={"flex"} flexDirection={"row-reverse"}>
                        <Grid size={4} sx={{ mb: 2 }} justifyItems={"right"}>
                            <Switch id="zone2" checked={acState.zone2} onChange={handleZoneChanged}></Switch>
                        </Grid>
                        <Grid size={8} justifyItems={"right"} >
                            <Typography variant='h6'>{zoneLabels[2]}</Typography>
                        </Grid>
                    </Grid>
                </Grid>

                <Grid size={6} justifyItems={"right"} display={"flex"} flexDirection={"column"}>
                    <Grid size={12} justifyItems={"right"} display={"flex"} flexDirection={"row-reverse"}>
                        <Grid size={5} sx={{ mb: 2 }} justifyItems={"right"}>
                            <Switch id="zone3" checked={acState.zone3} onChange={handleZoneChanged}></Switch>
                        </Grid>
                        <Grid size={7} justifyItems={"right"} >
                            <Typography variant='h6'>{zoneLabels[3]}</Typography>
                        </Grid>
                    </Grid>

                    <Grid size={12} justifyItems={"right"} display={"flex"} flexDirection={"row-reverse"}>
                        <Grid size={5} sx={{ mb: 2 }} justifyItems={"right"}>
                            <Switch id="zone4" checked={acState.zone4} onChange={handleZoneChanged}></Switch>
                        </Grid>
                        <Grid size={7} justifyItems={"right"} >
                            <Typography variant='h6'>{zoneLabels[4]}</Typography>
                        </Grid>
                    </Grid>

                    <Grid size={12} justifyItems={"right"} display={"flex"} flexDirection={"row-reverse"}>
                        <Grid size={5} sx={{ mb: 2 }} justifyItems={"right"}>
                            <Switch id="zone5" checked={acState.zone5} onChange={handleZoneChanged}></Switch>
                        </Grid>
                        <Grid size={7} justifyItems={"right"} >
                            <Typography variant='h6'>{zoneLabels[5]}</Typography>
                        </Grid>
                    </Grid>
                </Grid>
            </Grid>
        </Paper>
    );
}
