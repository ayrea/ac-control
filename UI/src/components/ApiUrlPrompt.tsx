import { Box, Button, TextField } from '@mui/material';
import { useState } from 'react';

export interface ApiUrlPromptProps {
    setBaseUrl: React.Dispatch<React.SetStateAction<string | null>>
}

export default function ApiUrlPrompt(props: ApiUrlPromptProps) {
    const [url, setUrl] = useState<string | null>(null);

    const handleSaveClicked = () => {
        if (url) {
            localStorage.setItem('baseApiUrl', url);
            props.setBaseUrl(url);
        }
    };

    const handleUrlChanged = (e: React.ChangeEvent<HTMLInputElement>) => {
        setUrl(e.target.value);
    }

    return (
        <Box display={"flex"}>
            <TextField id="api-url" label="API URL" variant="outlined" value={url} onChange={handleUrlChanged} />
            <Box m={1}>
                <Button variant='contained' disabled={!url} onClick={handleSaveClicked}>Save</Button>
            </Box>
        </Box>
    );
}
